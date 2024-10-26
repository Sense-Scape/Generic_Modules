#include "TimeChunkSynchronisationModule.h"
#include <algorithm>
#include <limits>
#include <chrono>

TimeChunkSynchronisationModule::TimeChunkSynchronisationModule(unsigned uBufferSize, uint64_t u64Threshold_ns, uint64_t u64SyncInterval_ns)
    : BaseModule(uBufferSize), m_u64Threshold_ns(u64Threshold_ns), m_u64SyncInterval_ns(u64SyncInterval_ns),
      m_tpLastSyncAttempt(std::chrono::steady_clock::now())
{
    RegisterChunkCallbackFunction(ChunkType::TimeChunk, &TimeChunkSynchronisationModule::Process_TimeChunk, (BaseModule*)this);
}

bool TimeChunkSynchronisationModule::IsDataContinuous(const std::vector<uint8_t>& sourceId, int64_t i64MostRecentTimeStamp, size_t uNumSamples, double dSampleRate_hz)
{
    auto it = m_MostRecentSourceTimestamp.find(sourceId);
    if (it == m_MostRecentSourceTimestamp.end())
        return true;

    int64_t i64LastReceivedDataTimestamp = it->second;
    int64_t i64ExpectedTimestamp = i64LastReceivedDataTimestamp + static_cast<int64_t>((uNumSamples) / dSampleRate_hz);
    int64_t i64ErrorBound_ns = 1e6; 

    bool bIsDataContinuous = std::abs(i64MostRecentTimeStamp - i64ExpectedTimestamp) <= i64ErrorBound_ns;

    if (!bIsDataContinuous)
        PLOG_WARNING << "Data is not continuous for source " << sourceId << ". Expected timestamp: " << i64ExpectedTimestamp << " ns, but received: " << i64MostRecentTimeStamp << " ns.";

    return bIsDataContinuous;
}

void TimeChunkSynchronisationModule::Process_TimeChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    auto pTimeChunk = std::static_pointer_cast<TimeChunk>(pBaseChunk);

    const auto& vu8SourceId = pTimeChunk->GetSourceIdentifier();
    double dSampleRate = pTimeChunk->m_dSampleRate;
    const auto& vi16FirstChannelData = pTimeChunk->m_vvi16TimeChunks[0];

    // Check if this is the first piece of data for this source
    // So we can make sure the channels are time aligned
    int64_t i64MostRecentTimeStamp = pTimeChunk->m_i64TimeStamp;
    if (m_TimeDataSourceMap[vu8SourceId].empty())
        m_OldestSourceTimestampMap[vu8SourceId] = i64MostRecentTimeStamp;

    // Check if the new timestamp is continuous
    if (!IsDataContinuous(vu8SourceId, i64MostRecentTimeStamp, vi16FirstChannelData.size(), dSampleRate))
    {
        ClearState();
        return;
    }

    StoreData(pTimeChunk);

    // Check if we have not received from other channels
    if (HasChannelTimeoutOccured())
    {
        ClearState();
        return;
    }

     // Check if we have enough data to perform synchronization
    if (!CheckQueuesHaveDataForMultilateration())
        return;

    // // Then check if we should try to synchronize the channels
    if (!ShouldWeTrySynchronise())
        return;

    SynchronizeChannels();

    // Check if we have enough data in all queues (e.g., 1 second worth of data)
    if (!CheckQueuesHaveEnoughData(30.0))
        return;

    // Package send and clear state
    auto pTDOATimeChunk = CreateSynchronizedTimeChunk();
    assert(pTDOATimeChunk != nullptr);

    TryPassChunk(pTDOATimeChunk);
    ClearState();   
}

bool TimeChunkSynchronisationModule::HasChannelTimeoutOccured()
{
    auto now = std::chrono::system_clock::now();
    auto i64TimeSinceEpoch_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
   
    for (const auto& SourceTimeStampPair : m_MostRecentSourceTimestamp)
    {
        uint64_t i64TimeSinceLastData_ns = (i64TimeSinceEpoch_us - SourceTimeStampPair.second)*1e3;
        if (i64TimeSinceLastData_ns > m_u64Threshold_ns)
        {
            PLOG_WARNING << "No data on channel " << SourceTimeStampPair.first << " for " << i64TimeSinceLastData_ns << " ns. Clearing state.";
            return true;
        }       
    }

    return false;
}

bool TimeChunkSynchronisationModule::ShouldWeTrySynchronise()
{   

    // Check if we have waited long enough to perform synchronization
    auto tpNow = std::chrono::steady_clock::now();
    auto u64ElapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(tpNow - m_tpLastSyncAttempt).count();
    if (u64ElapsedNs <= m_u64SyncInterval_ns)
        return false;
        
    m_tpLastSyncAttempt = tpNow;

    return true;
}

bool TimeChunkSynchronisationModule::CheckQueuesHaveDataForMultilateration()
{

    // Check if we have enough sources to do multilateration
    bool bEnoughSourcesToDoMultilateration = m_TimeDataSourceMap.size() >= 3;
    if (!bEnoughSourcesToDoMultilateration) 
        return false;

    // Check if all queues have data
    for (const auto& queuePair : m_TimeDataSourceMap)
    {
        if (queuePair.second.empty())
            return false;
    }

    return true;
}

bool TimeChunkSynchronisationModule::CheckQueuesHaveEnoughData(double dSecondsRequired)
{
    if (m_TimeDataSourceMap.empty())
        return false;

    size_t uSamplesRequired = static_cast<size_t>(dSecondsRequired * m_dSampleRate_hz);

    for (const auto& queuePair : m_TimeDataSourceMap)
    {
        if (queuePair.second.size() < uSamplesRequired)
            return false;
    }

    return true;
}

void TimeChunkSynchronisationModule::SynchronizeChannels()
{   
    // Find the most recent 'oldest' timestamp
    uint64_t u64MostRecentStaterTimestamp = 0;
    for (const auto& pair : m_OldestSourceTimestampMap)
    {
        u64MostRecentStaterTimestamp = std::max(u64MostRecentStaterTimestamp, pair.second);
    }

    // Calculate and remove excess samples from each channel
    for (auto& pair : m_TimeDataSourceMap)
    {
        const auto& sourceId = pair.first;
        auto& timeData = pair.second;

        int64_t i64TimeDifference = u64MostRecentStaterTimestamp - m_OldestSourceTimestampMap[sourceId];
        auto u32SamplesToRemove = (uint32_t)(i64TimeDifference/ 1e6) * m_dSampleRate_hz;

        if (u32SamplesToRemove > 0 && u32SamplesToRemove < timeData.size())
        {
            m_OldestSourceTimestampMap[sourceId] = u64MostRecentStaterTimestamp;
            timeData.erase(timeData.begin(), timeData.begin() + u32SamplesToRemove);
        }

    }
}

void TimeChunkSynchronisationModule::ClearState()
{
    m_TimeDataSourceMap.clear();
    m_OldestSourceTimestampMap.clear();
    m_MostRecentSourceTimestamp.clear();
    m_dSampleRate_hz = 0;
    m_tpLastSyncAttempt = std::chrono::steady_clock::now();
}

std::shared_ptr<TimeChunk> TimeChunkSynchronisationModule::CreateSynchronizedTimeChunk()
{
    if (m_TimeDataSourceMap.empty())
        return nullptr;

    // Determine the number of samples and channels
    size_t numSamples = m_TimeDataSourceMap.begin()->second.size();
    size_t numChannels = m_TimeDataSourceMap.size();

    if (numSamples == 0)
        return nullptr;

    // Create a new TimeChunk
    auto pSyncedTimeChunk = std::make_shared<TimeChunk>(
        static_cast<double>(numSamples),
        m_dSampleRate_hz,
        m_OldestSourceTimestampMap.begin()->second, // Use the oldest timestamp as the chunk timestamp
        16, // Assuming 16-bit samples, adjust if necessary
        2,  // Assuming 2 bytes per sample, adjust if necessary
        static_cast<unsigned>(numChannels)
    );

    // Fill the TimeChunk with synchronized data
    auto channelIndex = 0;
    pSyncedTimeChunk->m_vvi16TimeChunks.resize(numChannels);

    for (const auto& [sourceId, sourceData] : m_TimeDataSourceMap)
    {
        pSyncedTimeChunk->m_vvi16TimeChunks[channelIndex].resize(numSamples);
        std::copy_n(sourceData.begin(), numSamples, pSyncedTimeChunk->m_vvi16TimeChunks[channelIndex].begin());
        channelIndex++;
    }

    // Set the source identifier (assuming we want to use the first source's identifier)
    if (!m_TimeDataSourceMap.empty())
    {
        const auto& firstSource = m_TimeDataSourceMap.begin()->first;
        pSyncedTimeChunk->SetSourceIdentifier(firstSource);
    }

    return pSyncedTimeChunk;
}

void TimeChunkSynchronisationModule::StoreData(std::shared_ptr<TimeChunk> pTimeChunk)
{
    auto vu8SourceIdentifier = pTimeChunk->GetSourceIdentifier();

    m_TimeDataSourceMap[vu8SourceIdentifier] = pTimeChunk->m_vvi16TimeChunks[0];
    m_MostRecentSourceTimestamp[vu8SourceIdentifier] = pTimeChunk->m_i64TimeStamp;
    m_OldestSourceTimestampMap[vu8SourceIdentifier] = pTimeChunk->m_i64TimeStamp;
    m_dSampleRate_hz = pTimeChunk->m_dSampleRate;
}