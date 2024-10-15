#include "TimeChunkSynchronisationModule.h"
#include <algorithm>
#include <limits>
#include <chrono>

TimeChunkSynchronisationModule::TimeChunkSynchronisationModule(unsigned uBufferSize, uint64_t u64ThresholdNs, uint64_t u64SyncIntervalNs)
    : BaseModule(uBufferSize), m_u64ThresholdNs(u64ThresholdNs), m_u64SyncIntervalNs(u64SyncIntervalNs),
      m_tpLastSyncAttempt(std::chrono::steady_clock::now())
{
    RegisterChunkCallbackFunction(ChunkType::TimeChunk, &TimeChunkSynchronisationModule::Process_TimeChunk, (BaseModule*)this);
}

void TimeChunkSynchronisationModule::Process_TimeChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    auto pTimeChunk = std::static_pointer_cast<TimeChunk>(pBaseChunk);

    // Check if this is the first piece of data for this source
    // So we can make sure the channels are time aligned
    if (m_TimeDataSourceMap[pTimeChunk->GetSourceIdentifier()].empty())
        m_OldestSourceTimestampMap[pTimeChunk->GetSourceIdentifier()] = pTimeChunk->m_i64TimeStamp;

    // Store time data instead of only the first channel of data
    auto& vi16TimeData = m_TimeDataSourceMap[pTimeChunk->GetSourceIdentifier()];
    vi16TimeData.insert(vi16TimeData.end(), pTimeChunk->m_vvi16TimeChunks[0].begin(), pTimeChunk->m_vvi16TimeChunks[0].end());

    // Then store the timestamp and sample rate of data (assuming sample rate is constant for all data)
    m_MostRecentSourceTimestamp[pTimeChunk->GetSourceIdentifier()] = pTimeChunk->m_i64TimeStamp;
    m_u64SampleRate_hz = pTimeChunk->m_i64TimeStamp;

    if(!ShouldWeTrySynchronise());
        return;
    
    SynchronizeChannels();

}

bool TimeChunkSynchronisationModule::ShouldWeTrySynchronise()
{   
    // Check if we have enough data to perform synchronization
    if (!CheckAllQueuesHaveData())
        return false;

    // Check if we have enough sources to do multilateration
    bool bEnoughSourcesToDoMultilateration = m_TimeDataSourceMap.size() >= 3;
    if (!bEnoughSourcesToDoMultilateration) 
        return false;

    // Check if we have waited long enough to perform synchronization
    auto tpNow = std::chrono::steady_clock::now();
    auto u64ElapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(tpNow - m_tpLastSyncAttempt).count();
    if (u64ElapsedNs <= m_u64SyncIntervalNs)
        return false;
    m_tpLastSyncAttempt = tpNow;

    return true;
}

bool TimeChunkSynchronisationModule::CheckAllQueuesHaveData()
{
    for (const auto& queuePair : m_TimeDataSourceMap)
    {
        if (queuePair.second.empty())
        {
            return false;
        }
    }
    return true;
}

void TimeChunkSynchronisationModule::SynchronizeChannels()
{
    // Find the most recent 'oldest' timestamp
    uint64_t i64MostRecentOldestTimestamp = std::numeric_limits<int64_t>::min();
    for (const auto& pair : m_OldestSourceTimestampMap)
        i64MostRecentOldestTimestamp = std::max(i64MostRecentOldestTimestamp, pair.second);

    // Calculate and remove excess samples from each channel
    for (auto& pair : m_TimeDataSourceMap)
    {
        const auto& sourceId = pair.first;
        auto& timeData = pair.second;

        int64_t i64TimeDifference = i64MostRecentOldestTimestamp - m_OldestSourceTimestampMap[sourceId];
        size_t uSamplesToRemove = static_cast<size_t>(i64TimeDifference * m_u64SampleRate_hz / 1e9);

        if (uSamplesToRemove > 0 && uSamplesToRemove < timeData.size())
        {
            timeData.erase(timeData.begin(), timeData.begin() + uSamplesToRemove);
            m_OldestSourceTimestampMap[sourceId] = i64MostRecentOldestTimestamp;
        }
    }
}
