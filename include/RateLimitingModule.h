#ifndef RATE_LIMITING_MODULE
#define RATE_LIMITING_MODULE

#include "BaseModule.h"
#include "chrono"

/**
 * @brief Limits transmission of chunks according to chunk type and source identifier
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
     * @param pBaseChunk pointer to chunk to process
     */
    void Process_Chunk(std::shared_ptr<BaseChunk> pBaseChunk);

    /**
     * @brief Sets the limiting rate for chunk type
     * @param eChunkType enumerated chunk to to limit
     * @param u32ReportPeriod how often the chunk is allowed to propogate in the pipeline
     */
    void SetChunkRateLimitInUsec(ChunkType eChunkType, uint32_t u32ReportPeriod);

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    std::string GetModuleType() override { return "RateLimitingModule"; };

private:

    std::map<ChunkType,uint64_t> m_mapChunkTypeToRatePeriod;    ///< Map which stores which chunk should be rate limited
    std::map<std::vector<uint8_t>,std::map<ChunkType,uint64_t>> m_mapChunkTypeToLastReportTime;  ///< Map which stores when the last chunk was sent 

};

#endif