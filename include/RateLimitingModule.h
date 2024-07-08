#ifndef RATE_LIMITING_MODULE
#define RATE_LIMITING_MODULE

#include "BaseModule.h"
#include "chrono"

/**
 * @brief Converts just to the JSON Chunk type
 */
class RateLimitingModule : public BaseModule
{
public:
    /**
     * @brief Construct a new RateLimitingModule object
     */
    RateLimitingModule(unsigned uBufferSize);
    //~RateLimitingModule(){};

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    void SetChunkRateLimitInUsec(ChunkType eChunkType, uint32_t u32ReportPeriod);

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    std::string GetModuleType() override { return "RateLimitingModule"; };

private:

    std::map<ChunkType,uint64_t> m_mapChunkTypeToRatePeriod;    ///< Map which stores which chunk should be rate limited
    std::map<ChunkType,uint64_t> m_mapChunkTypeToLastReportTime;  ///< Map which stores when the last chunk was sent 

};

#endif