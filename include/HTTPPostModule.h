#ifndef HTTP_POST_MODULE
#define HTTP_POST_MODULE

#include "BaseModule.h"
#include "ChunkToJSONConverter.h"
#include "JSONChunk.h"

#include <chrono>
#include <httplib.h>
#include <map>
#include <string>

/**
 * @brief Converts incoming chunks (JSON or convertible) and posts them to an HTTP endpoint.
 */
class HTTPPostModule : public BaseModule
{
public:
    /**
     * @brief Construct a new HTTPPostModule object
     * @param uBufferSize Maximum number of queued chunks
     * @param jsonConfig JSON configuration block for this module
     */
    HTTPPostModule(unsigned uBufferSize, const nlohmann::json_abi_v3_11_2::json& jsonConfig);

    /**
     * @brief Returns module type
     */
    std::string GetModuleType() override { return "HTTPPostModule"; };

    /**
     * @brief Updates the endpoint at runtime (scheme://host[:port]/path)
     */
    void SetEndpoint(const std::string& strEndpoint);

private:

    struct EndpointDetails
    {
        std::string scheme; ///< The protocol used for the connection (e.g., "http" or "https").
        std::string host;   ///< The domain name or IP address of the target server (e.g., "api.example.com").
        int port;           ///< The port number on which the service is listening (e.g., 80, 443, or 8080).
        std::string path;   ///< The specific resource path on the server (e.g., "/users/profile").
    };

    /**
     * @brief use a json object to configure this module
     * @param[in] jsonConfig JSON configuration of this module
     */
    void ConfigureModuleJSON(const nlohmann::json_abi_v3_11_2::json& jsonConfig);

    void Process_JSONChunk(std::shared_ptr<BaseChunk> pBaseChunk);
    bool SendBody(const std::string& strBody, const httplib::Headers& headers);
    

    /**
     * @brief Parses a full endpoint URL string into the scheme (protocol), host (domain/IP), port number, and resource path..
     * @param strEndpoint The full endpoint URL string to parse (e.g., "https://api.example.com:8080/v1/data").
     * @return EndpointDetails A structure containing the separated components: scheme, host, port, and path.
     */
    EndpointDetails ParseEndpoint(const std::string& strEndpoint);

    // Defined to allow both http and https clients
    template<typename ClientType>
    void ConfigureClient(ClientType& client);

    bool m_bUseSSL;                                     ///< Whether to use ssl or not
    EndpointDetails m_sEndpoint;                        ///< Endpoint to which we post
    std::string m_strContentType = "application/json";  ///< Whether we are sending json messages - other types unsupported
    std::chrono::milliseconds m_ConnectionTimeout;      ///< How long a http/s client waits while trying to establish tcp connection
    std::chrono::milliseconds m_ReadTimeout;            ///< How long a http/s client waits before timing out on a read
    std::chrono::milliseconds m_WriteTimeout;           ///< How long a http/s client waits before timing out on a write
    std::chrono::milliseconds m_RetryBackoff;           ///< How long to wait before retrying after a failed post
    uint32_t m_uMaxRetries;                             ///< How many post retries before failure
};

template<typename ClientType>
void HTTPPostModule::ConfigureClient(ClientType& client)
{
    client.set_connection_timeout(m_ConnectionTimeout);
    client.set_read_timeout(m_ReadTimeout);
    client.set_write_timeout(m_WriteTimeout);
    client.set_follow_location(true);
    client.set_decompress(true);
}

#endif

