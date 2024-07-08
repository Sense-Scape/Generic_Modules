#include "RateLimitingModule.h"


RateLimitingModule::RateLimitingModule(unsigned uBufferSize) : BaseModule(uBufferSize) 
{
}

void RateLimitingModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // First check if we need to rate limit this chunk
    auto eChunkType = pBaseChunk->GetChunkType();
    if(m_mapChunkTypeToRatePeriod.find(eChunkType) == m_mapChunkTypeToRatePeriod.end())
        TryPassChunk(pBaseChunk);

    // If we do then check the period
    auto u64Now = std::chrono::system_clock::now().time_since_epoch().count();
    auto u64elapsedTime_ns = u64Now - m_mapChunkTypeToLastReportTime[eChunkType];

    // and see when last we sent it
    if(u64elapsedTime_ns >= m_mapChunkTypeToRatePeriod[eChunkType])
    {
        m_mapChunkTypeToLastReportTime[eChunkType] = u64Now;
        TryPassChunk(pBaseChunk);
    }
}

void RateLimitingModule::SetChunkRateLimitInUsec(ChunkType eChunkType, uint32_t u32ReportPeriod)
{
    m_mapChunkTypeToRatePeriod[eChunkType] = u32ReportPeriod;
    
    std::string strInfo = ChunkTypesNamingUtility::toString(eChunkType) + " is being rate limited to ns period of " + std::to_string(u32ReportPeriod);
    PLOG_INFO << strInfo;
}