#ifndef HTTP_POST_MODULE
#define HTTP_POST_MODULE

#include "BaseModule.h"

#include <chrono>
#include <curl/curl.h>
#include <string>

/**
 * @brief Converts incoming chunks (JSON or convertible) and posts them to an
 * HTTP endpoint.
 */
class HTTPPostModule : public BaseModule {
public:
  /**
   * @brief Construct a new HTTPPostModule object
   * @param uBufferSize Maximum number of queued chunks
   * @param jsonConfig JSON configuration block for this module
   */
  HTTPPostModule(unsigned uBufferSize,
                 const nlohmann::json_abi_v3_11_2::json &jsonConfig);
  ~HTTPPostModule();

  /**
   * @brief Returns module type
   */
  std::string GetModuleType() override { return "HTTPPostModule"; };

private:
  void Process_JSONChunk(std::shared_ptr<BaseChunk> pBaseChunk);
  bool SendBody(std::shared_ptr<BaseChunk> pBaseChunk);

  // Defined to allow both http and https clients
  template <typename ClientType> void ConfigureClient(ClientType &client);

  const bool m_bUseSSL;            ///< Whether to use ssl or not
  const std::string m_strEndpoint; ///< Endpoint to which we post
  const std::string m_strContentType =
      "application/json"; ///< Whether we are sending json messages - other
                          ///< types unsupported
  const std::chrono::milliseconds
      m_ConnectionTimeout; ///< How long a http/s client waits while trying to
                           ///< establish tcp connection
  const std::chrono::milliseconds
      m_ReadTimeout; ///< How long a http/s client waits
                     ///< before timing out on a read
  const std::chrono::milliseconds
      m_WriteTimeout; ///< How long a http/s client waits
                      ///< before timing out on a write
  const std::chrono::milliseconds
      m_RetryBackoff; ///< How long to wait before retrying after a failed post
  const uint32_t m_uMaxRetries; ///< How many post retries before failure
  CURL *m_pCurl;
  uint32_t m_u32ErrorLogCounter;
};

#endif
