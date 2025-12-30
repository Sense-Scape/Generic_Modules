#include "HTTPPostModule.h"
#include "plog/Log.h"
#include <string>

// Callback function for libcurl to write response data
static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  size_t totalSize = size * nmemb;
  std::string *response = static_cast<std::string *>(userp);
  response->append(static_cast<char *>(contents), totalSize);
  return totalSize;
}

HTTPPostModule::HTTPPostModule(
    unsigned uBufferSize, const nlohmann::json_abi_v3_11_2::json &jsonConfig)
    : BaseModule(uBufferSize), m_bUseSSL(false),
      m_ConnectionTimeout(std::chrono::seconds(5)),
      m_ReadTimeout(std::chrono::seconds(5)),
      m_WriteTimeout(std::chrono::seconds(5)), m_uMaxRetries(2),
      m_RetryBackoff(std::chrono::milliseconds(250)), m_pCurl(nullptr),
      m_u32ErrorLogCounter(0),
      m_strEndpoint(CheckAndThrowJSON<std::string>(jsonConfig, "EndPoint")) {

  curl_global_init(CURL_GLOBAL_DEFAULT);
  m_pCurl = curl_easy_init();

  RegisterChunkCallbackFunction(ChunkType::JSONChunk,
                                &HTTPPostModule::Process_JSONChunk,
                                (BaseModule *)this);
}

HTTPPostModule::~HTTPPostModule() {
  // Cleanup curl handle
  if (m_pCurl) {
    curl_easy_cleanup(m_pCurl);
    m_pCurl = nullptr;
  }

  // Cleanup libcurl globally (should be done once per application at exit)
  curl_global_cleanup();
}

void HTTPPostModule::Process_JSONChunk(std::shared_ptr<BaseChunk> pBaseChunk) {
  // Now keep trying to send data
  for (uint32_t attempt = 0; attempt <= m_uMaxRetries; ++attempt) {
    if (SendBody(pBaseChunk))
      return;

    if (attempt < m_uMaxRetries)
      std::this_thread::sleep_for(m_RetryBackoff);
  }
}

bool HTTPPostModule::SendBody(std::shared_ptr<BaseChunk> pBaseChunk) {
  auto pJSONChunk = std::dynamic_pointer_cast<JSONChunk>(pBaseChunk);
  nlohmann::json jsonDocument = pJSONChunk->m_JSONDocument;

  std::string strBody = jsonDocument.dump();

  // Reinitialize curl handle if needed
  if (!m_pCurl) {
    PLOG_INFO << "Curl handle not initialized, creating new handle";
    m_pCurl = curl_easy_init();
    if (!m_pCurl) {
      PLOG_ERROR << "Failed to initialize curl handle";
      return false;
    }
  }

  // Response string to capture the response body
  std::string responseBody;

  // Setup curl options
  curl_easy_setopt(m_pCurl, CURLOPT_URL, m_strEndpoint.c_str());
  curl_easy_setopt(m_pCurl, CURLOPT_POST, 1L);
  curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, strBody.c_str());
  curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, strBody.length());

  // Set headers
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);

  // Set timeouts
  curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(m_pCurl, CURLOPT_TIMEOUT, 30L);

  // Enable keep-alive
  curl_easy_setopt(m_pCurl, CURLOPT_TCP_KEEPALIVE, 1L);

  // Set callback to capture response
  curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &responseBody);

  // Perform the request
  CURLcode res = curl_easy_perform(m_pCurl);

  // Cleanup headers
  curl_slist_free_all(headers);

  if (res != CURLE_OK) {
    PLOG_ERROR << "HTTP Error: " << m_strEndpoint << " "
               << curl_easy_strerror(res);
    return false;
  }

  // Get HTTP response code
  long httpCode = 0;
  curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, &httpCode);

  bool bSuccess = httpCode >= 200 && httpCode < 300;
  if (!bSuccess && m_u32ErrorLogCounter == 0) {
    PLOG_ERROR << "HTTP Code: " << httpCode << " with response " << responseBody
               << " from " << m_strEndpoint;
    m_u32ErrorLogCounter++;
  } else if (m_u32ErrorLogCounter < 50)
    m_u32ErrorLogCounter++;
  else
    m_u32ErrorLogCounter = 0;

  return bSuccess;
}
