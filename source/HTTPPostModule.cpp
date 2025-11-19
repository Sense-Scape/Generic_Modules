#include "HTTPPostModule.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace
{
bool EqualsIgnoreCase(const std::string& lhs, const std::string& rhs)
{
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](char a, char b)
           {
               return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
           });
}

std::string TrimCopy(const std::string& input)
{
    const auto first = input.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return "";
    const auto last = input.find_last_not_of(" \t\n\r");
    return input.substr(first, last - first + 1);
}
}

HTTPPostModule::HTTPPostModule(unsigned uBufferSize, const nlohmann::json_abi_v3_11_2::json& jsonConfig)
    : BaseModule(uBufferSize),
      m_bUseSSL(false),
      m_ConnectionTimeout(std::chrono::seconds(5)),
      m_ReadTimeout(std::chrono::seconds(5)),
      m_WriteTimeout(std::chrono::seconds(5)),
      m_uMaxRetries(2),
      m_RetryBackoff(std::chrono::milliseconds(250))
      
{
    ConfigureModuleJSON(jsonConfig);

    RegisterChunkCallbackFunction(ChunkType::JSONChunk, &HTTPPostModule::Process_JSONChunk, (BaseModule*)this);
}

void HTTPPostModule::Process_JSONChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{

    // First try convert a json chunk
    nlohmann::json_abi_v3_11_2::json jsonDocument;
    auto pJSONChunk = std::dynamic_pointer_cast<JSONChunk>(pBaseChunk);
    jsonDocument = pJSONChunk->m_JSONDocument;

    // First create an empty header then update it and get the data to send
    httplib::Headers headers;
    headers.emplace("Content-Type", m_strContentType);
    const std::string strBody = jsonDocument.dump();

    // Now keep trying to send data
    for (uint32_t attempt = 0; attempt <= m_uMaxRetries; ++attempt)
    {
        if (SendBody(strBody, headers))
            return;

        if (attempt < m_uMaxRetries)
            std::this_thread::sleep_for(m_RetryBackoff);
    }

    // If we didnt then sads
    PLOG_WARNING << "HTTPPostModule failed to POST payload to " << m_sEndpoint.host << ":" << m_sEndpoint.port << m_sEndpoint.path;        
}

void HTTPPostModule::SetEndpoint(const std::string& strEndpoint)
{
    m_sEndpoint = ParseEndpoint(strEndpoint);
    m_bUseSSL = (m_sEndpoint.scheme == "https");
    std::stringstream ssInfo;
    ssInfo << std::string(__FUNCTION__) << "HTTPPostModule configured for " << m_sEndpoint.scheme << "://" << m_sEndpoint.host << ":" << m_sEndpoint.port << m_sEndpoint.path;
    PLOG_INFO << ssInfo.str();
}

HTTPPostModule::EndpointDetails HTTPPostModule::ParseEndpoint(const std::string& strEndpoint)
{
    EndpointDetails endpoint{};
    const std::string trimmed = TrimCopy(strEndpoint);
    const auto schemePosition = trimmed.find("://");

    // We first make sure there is a scheme
    if (schemePosition == std::string::npos)
        throw std::runtime_error("HTTPPostModule endpoint is missing scheme");

    // Then we make sure the scheme is lowercase
    endpoint.scheme = trimmed.substr(0, schemePosition);
    std::transform(endpoint.scheme.begin(), endpoint.scheme.end(), endpoint.scheme.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });

    const size_t authorityStart = schemePosition + 3;
    const size_t pathStart = trimmed.find('/', authorityStart);
    const std::string authority = (pathStart == std::string::npos) ? trimmed.substr(authorityStart)
                                                                    : trimmed.substr(authorityStart, pathStart - authorityStart);
    
    // Then we find out where we are actually transmitting the data
    endpoint.path = (pathStart == std::string::npos) ? "/" : trimmed.substr(pathStart);

    const auto colonPos = authority.find(':');
    if (colonPos == std::string::npos)
    {
        endpoint.host = authority;
        endpoint.port = (endpoint.scheme == "https") ? 443 : 80;
    }
    else
    {
        endpoint.host = authority.substr(0, colonPos);
        const auto strPort = authority.substr(colonPos + 1);
        endpoint.port = std::stoi(strPort);
    }

    if (endpoint.host.empty())
        throw std::runtime_error("HTTPPostModule endpoint host cannot be empty");

    if (endpoint.port <= 0)
        throw std::runtime_error("HTTPPostModule endpoint port is invalid");

    return endpoint;
}

bool HTTPPostModule::SendBody(const std::string& strBody, const httplib::Headers& headers)
{
    const auto logTarget = m_sEndpoint.scheme + "://" + m_sEndpoint.host + ":" + std::to_string(m_sEndpoint.port) + m_sEndpoint.path;

    // We define a lambda here so we can agnostically call rsuse this code for both a HTTP and HTTPS client
    auto postWithClient = [&](auto& client) -> bool
    {
        ConfigureClient(client);
        auto result = client.Post(m_sEndpoint.path.c_str(), headers, strBody, m_strContentType.c_str());

        if (!result)
        {
            std::string strWarning = std::string(__FUNCTION__) + "HTTPPostModule unable to reach " + logTarget + " error=" + httplib::to_string(result.error());
            PLOG_WARNING << strWarning;
            return false;
        }

        if (result->status >= 200 && result->status < 300)
        {   
            std::stringstream ssInfo;
            ssInfo << std::string(__FUNCTION__) << "HTTPPostModule posted payload to " << logTarget << " status=" << result->status;
            PLOG_INFO << ssInfo.str();
            return true;
        }

        std::string strWarning = "HTTPPostModule received non-success response (" + std::to_string(result->status) + ") from " + logTarget;
        PLOG_WARNING << strWarning;
        return false;
    };

    if (m_bUseSSL)
    {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        httplib::SSLClient client(m_sEndpoint.host, m_sEndpoint.port);
        client.enable_server_certificate_verification(true);
        return postWithClient(client);
#else
        PLOG_ERROR << "HTTPPostModule configured for HTTPS but cpp-httplib lacks OpenSSL support";
        return false;
#endif
    }

    // Then create the httplib client which we then
    // update with our ConfigureClient function
    httplib::Client client(m_sEndpoint.host, m_sEndpoint.port);
    return postWithClient(client);
}

void HTTPPostModule::ConfigureModuleJSON(const nlohmann::json_abi_v3_11_2::json& jsonConfig)
{
    std::string strInfo = std::string(__FUNCTION__) + ": No JSON config required for http post module";
    PLOG_INFO << strInfo;
}

