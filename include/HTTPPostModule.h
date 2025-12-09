#ifndef HTTP_POST_MODULE
#define HTTP_POST_MODULE

#include "BaseModule.h"
#include "ChunkToJSONConverter.h"
#include "JSONChunk.h"

#include <chrono>
#include <httplib.h>
#include <map>
#include <string>
#include <curl/curl.h>

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
    HTTPPostModule(unsigned uBufferSize,std::string strEndpoint, const nlohmann::json_abi_v3_11_2::json& jsonConfig);
    ~HTTPPostModule();
    
    /**
     * @brief Returns module type
     */
    std::string GetModuleType() override { return "HTTPPostModule"; };

    /**
     * @brief Updates the endpoint at runtime (scheme://host[:port]/path)
     */
    void SetEndpoint(const std::string strEndpoint);

private:

    /**
     * @brief use a json object to configure this module
     * @param[in] jsonConfig JSON configuration of this module
     */
    void ConfigureModuleJSON(const nlohmann::json_abi_v3_11_2::json& jsonConfig);

    void Process_JSONChunk(std::shared_ptr<BaseChunk> pBaseChunk);
    bool SendBody(std::shared_ptr<BaseChunk> pBaseChunk);
    
    // Defined to allow both http and https clients
    template<typename ClientType>
    void ConfigureClient(ClientType& client);

    bool m_bUseSSL;                                     ///< Whether to use ssl or not
    std::string m_strEndpoint;                        ///< Endpoint to which we post
    std::string m_strContentType = "application/json";  ///< Whether we are sending json messages - other types unsupported
    std::chrono::milliseconds m_ConnectionTimeout;      ///< How long a http/s client waits while trying to establish tcp connection
    std::chrono::milliseconds m_ReadTimeout;            ///< How long a http/s client waits before timing out on a read
    std::chrono::milliseconds m_WriteTimeout;           ///< How long a http/s client waits before timing out on a write
    std::chrono::milliseconds m_RetryBackoff;           ///< How long to wait before retrying after a failed post
    uint32_t m_uMaxRetries;                             ///< How many post retries before failure
    CURL* m_pCurl;
    uint32_t m_u32ErrorLogCounter;
};

#endif

