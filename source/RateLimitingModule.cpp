#include "RateLimitingModule.h"


RateLimitingModule::RateLimitingModule(unsigned uBufferSize) : BaseModule(uBufferSize) 
{
}

void RateLimitingModule::Process_Chunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // First check if we need to rate limit this chunk
    auto vu8SourceIdentifier = pBaseChunk->GetSourceIdentifier();
    auto eChunkType = pBaseChunk->GetChunkType();

    // If we do then check the period
    auto u64Now = std::chrono::system_clock::now().time_since_epoch().count();
    auto u64elapsedTime_ns = u64Now - m_mapChunkTypeToLastReportTime[vu8SourceIdentifier][eChunkType];

    // and see when last we sent it
    if(u64elapsedTime_ns >= m_mapChunkTypeToRatePeriod[eChunkType])
    {
        m_mapChunkTypeToLastReportTime[vu8SourceIdentifier][eChunkType] = u64Now;
        TryPassChunk(pBaseChunk);
    }
}

void RateLimitingModule::SetChunkRateLimitInUsec(ChunkType eChunkType, uint32_t u32ReportPeriod)
{
    m_mapChunkTypeToRatePeriod[eChunkType] = u32ReportPeriod;
    RegisterChunkCallbackFunction(eChunkType, &RateLimitingModule::Process_Chunk,(BaseModule*)this);

    std::string strInfo = ChunkTypesNamingUtility::toString(eChunkType) + " is being rate limited to ns period of " + std::to_string(u32ReportPeriod);
    PLOG_INFO << strInfo;
}