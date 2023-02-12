#ifndef WATCHDOG_MODULE
#define WATCHDOG_MODULE

/* Custom Includes */
#include "BaseModule.h"
#include "WatchdogChunk.h"

class WatchdogModule :
    public BaseModule
{
public:
    /**
     * @brief Construct a new Session Processing Module to produce UDP data. responsible
     *  for acummulating all required UDP bytes and passing on for further processing
     * @param[in] uReportTime_s time between alive reports
     */
    WatchdogModule(unsigned uReportTime_s, std::vector<uint8_t> &vu8MACAddress);
    ~WatchdogModule() {};

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::WatchdogModule; };

    /**
     * @brief Check input buffer and try process data
     */
    void ContinuouslyTryProcess() override;

    /*
    * @brief Module process to write WAV file
    * @param[in] pBaseChunkpointer to base chunk
    */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

private:
    unsigned m_uReportTime_s;               ///< Seconds between alive reports
    std::vector<uint8_t> m_vu8MACAddress;   ///< MAC address in bytes 
};

#endif
