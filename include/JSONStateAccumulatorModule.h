#ifndef JSON_ACCUMULATOR_MODULE
#define JSON_ACCUMULATOR_MODULE

#include "BaseModule.h"
#include "JSONChunk.h"


/**
 * @brief Object which accumulates and transmits JSON documents from a sending sources
 */
class JSONStateAccumulatorModule : public BaseModule
{
public:
    /**
     * @brief Construct a new ToJSONModule object
     */
    JSONStateAccumulatorModule(unsigned uBufferSize,nlohmann::json_abi_v3_11_2::json jsonConfig);
    //~ToJSONModule(){};

    /**
     * @brief Calls the infinite loop to start processing
     */
    void StartProcessing() override;

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    std::string GetModuleType() override { return "JSONStateAccumulatorModule"; };

private:

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process_JSONChunk(std::shared_ptr<BaseChunk> pBaseChunk);

    /**
     * @brief Function which continuously waits and send current JSON document states
     */
    void WaitAndSendJSONDocuments();

    /**
     * @brief use a json object to configure this module
     * @param[in] jsonConfig JSON configuration of this module
     */
    void ConfigureModuleJSON(nlohmann::json_abi_v3_11_2::json jsonConfig);

    std::map<std::vector<uint8_t>, nlohmann::json_abi_v3_11_2::json> m_JSONStatesBySource;      ///< Map of source identifiers and JSON documents
    double m_dReportingPeriod_s;                                                                ///< How often (s) Json messages are sent
    std::thread m_QueueJSONTxThread;                                                            ///< Thread used to periodically send JSON messages
    bool m_bEnableJSONLogs;                                                                     ///< Bool whether to logs processed JSON documents

};

#endif