#include "TimeChunkSynchronisationModule.h"
#include <algorithm>
#include <limits>
#include <chrono>

TimeChunkSynchronisationModule::TimeChunkSynchronisationModule(unsigned uBufferSize, uint64_t u64Threshold_us, uint64_t u64SyncInterval_ns)
    : BaseModule(uBufferSize), m_u64ChannelDiscontinuityThreshold_us(u64Threshold_us), m_u64SyncInterval_ns(u64SyncInterval_ns),
      m_tpLastSyncAttempt(std::chrono::steady_clock::now())
{
    RegisterChunkCallbackFunction(ChunkType::TimeChunk, &TimeChunkSynchronisationModule::Process_TimeChunk, (BaseModule*)this);
}

bool TimeChunkSynchronisationModule::IsDataContinuous(const std::vector<uint8_t>& sourceId, int64_t i64MostRecentTimeStamp, size_t uNumSamples, double dSampleRate_hz)
{
    auto it = m_MostRecentSourceTimestamp.find(sourceId);
    if (it == m_MostRecentSourceTimestamp.end())
        return true;

    // Use the time of the most recently received time chunk
    int64_t i64LastReceivedDataTimestamp_us = it->second;
    int64_t i64ExpectedTimestamp_us = i64LastReceivedDataTimestamp_us + static_cast<int64_t>((1e6 * uNumSamples) / dSampleRate_hz);
    int64_t i64ErrorBound_us = 1e6; 

    bool bIsDataContinuous = std::abs(i64MostRecentTimeStamp - i64ExpectedTimestamp_us) <= i64ErrorBound_us;

    if (!bIsDataContinuous)
        PLOG_WARNING << "Data is not continuous for source " << sourceId << ". Expected timestamp: " << i64ExpectedTimestamp_us << " ns, but received: " << i64MostRecentTimeStamp << " ns.";

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
    TryInitialiseDataSource(pTimeChunk);

    int64_t i64MostRecentTimeStamp = pTimeChunk->m_i64TimeStamp;
    auto it = m_OldestSourceTimestampMap.find(vu8SourceId);
    if (it == m_OldestSourceTimestampMap.end())
    {
        PLOG_ERROR << "    Settingv Time_stamps    " << i64MostRecentTimeStamp;
        m_OldestSourceTimestampMap[vu8SourceId] = i64MostRecentTimeStamp;
    }
        

    bool bDataContinuous = IsDataContinuous(vu8SourceId, i64MostRecentTimeStamp, vi16FirstChannelData.size(), dSampleRate);
    bool bChannelCountConsistent = IsChannelCountTheSame(pTimeChunk);
    bool dataValid = bDataContinuous && bChannelCountConsistent;
    
    if (!dataValid)
    {
        ClearState();
        return;
    }

    StoreData(pTimeChunk);

    //Check if we have not received from other channels
    // if (HasChannelTimeoutOccured())
    // {
    //     ClearState();
    //     return;
    // }

    if (!CheckQueuesHaveDataForMultilateration())
        return;

    // // Then check if we should try to synchronize the channels
    if (!ShouldWeTrySynchronise())
        return;

    SynchronizeChannels();
    
    // Check if we have enough data in all queues (e.g., 1 second worth of data)
    if (!CheckQueuesHaveEnoughData())
    {
        return;
    }
    else
    {
        PLOG_WARNING << "Extracting TDOA data";
    }


    
    // Package send and clear state
    auto pTDOATimeChunk = CreateSynchronizedTimeChunk();
    assert(pTDOATimeChunk != nullptr);

    TryPassChunk(pTDOATimeChunk);  
}

bool TimeChunkSynchronisationModule::HasChannelTimeoutOccured()
{
    auto now = std::chrono::system_clock::now();
    auto i64TimeSinceEpoch_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
   
    for (const auto& SourceTimeStampPair : m_MostRecentSourceTimestamp)
    {

        int64_t i64TimeSinceLastData_us = std::fabs(i64TimeSinceEpoch_us - SourceTimeStampPair.second);

        if (i64TimeSinceLastData_us > m_u64ChannelDiscontinuityThreshold_us)
        {
            PLOG_WARNING << "No data on channel " << SourceTimeStampPair.first << " for " << i64TimeSinceLastData_us/1e6 << " seconds. Clearing state.";
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

bool TimeChunkSynchronisationModule::CheckQueuesHaveEnoughData()
{
    // Dont sync if we dont have data
    if (m_TimeDataSourceMap.empty())
        return false;
    
    size_t uSamplesRequired = static_cast<size_t>(m_TDOALength_s * m_dSampleRate_hz);
   
    for (const auto& queuePair : m_TimeDataSourceMap)
    {
        if (queuePair.second.size() <= uSamplesRequired)
            return false;
    }
    
    return true;
}

void TimeChunkSynchronisationModule::SynchronizeChannels()
{   
    // Find the most recent timestamp from all the channels
    uint64_t u64MostRecentStaterTimestamp = 0;
    for (const auto& pair : m_OldestSourceTimestampMap)
        u64MostRecentStaterTimestamp = std::max(u64MostRecentStaterTimestamp, pair.second);

    PLOG_ERROR << "    Time_stamps    ";
    for (const auto& pair : m_OldestSourceTimestampMap)
       PLOG_ERROR << pair.second ;
    PLOG_ERROR << "--------------------";
    auto count = 0;

    // Calculate and remove excess samples from each channel
    for (auto& pair : m_TimeDataSourceMap)
    {
        const auto& vu8SourceId = pair.first;
        auto& vu8TimeData = pair.second;

        int64_t i64TimeDifference = u64MostRecentStaterTimestamp - m_OldestSourceTimestampMap[vu8SourceId];

        PLOG_ERROR << "Time diff" << i64TimeDifference;

        auto u32SamplesToRemove = (uint32_t)(m_dSampleRate_hz*i64TimeDifference/ 1e6);

        if (u32SamplesToRemove > 0 && u32SamplesToRemove < vu8TimeData.size())
        {
            PLOG_ERROR << count << " Timestamp Diff: " << i64TimeDifference <<  "    Removing Samples:   " <<  u32SamplesToRemove << "   Updated Timestamp   " << u64MostRecentStaterTimestamp <<  std::endl;
            m_OldestSourceTimestampMap[vu8SourceId] = u64MostRecentStaterTimestamp;
            vu8TimeData.erase(vu8TimeData.begin(), vu8TimeData.begin() + u32SamplesToRemove);
        }
        else
        {
            PLOG_ERROR << count << " Keeping all data for this channel";
        }

         count++;

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

std::shared_ptr<TDOAChunk> TimeChunkSynchronisationModule::CreateSynchronizedTimeChunk()
{
    if (m_TimeDataSourceMap.empty())
        return nullptr;

    // Determine the number of samples and channels
    size_t numSamples = static_cast<size_t>(m_TDOALength_s * m_dSampleRate_hz);
    size_t numChannels = m_TimeDataSourceMap.size();

    if (numSamples == 0)
        return nullptr;

    auto pTDOAChunk = std::make_shared<TDOAChunk>(
        m_dSampleRate_hz,
        m_OldestSourceTimestampMap.begin()->second
    );

    // Fill the TimeChunk with synchronized data
    auto channelIndex = 0;

    for (const auto& [vu8SourceId, sourceData] : m_TimeDataSourceMap)
    {
       //pTDOAChunk->AddData(0,0,);
    }
    

    // Set the source identifier (assuming we want to use the first source's identifier)
    if (!m_TimeDataSourceMap.empty())
    {
        auto vu8firstSource = m_TimeDataSourceMap.begin()->first;
        pTDOAChunk->SetSourceIdentifier(vu8firstSource);
    }
    else
        PLOG_ERROR << "source not applied";

    return pTDOAChunk;
}

bool TimeChunkSynchronisationModule::IsChannelCountTheSame(std::shared_ptr<TimeChunk> pTimeChunk)
{
    auto vu8SourceIdentifier = pTimeChunk->GetSourceIdentifier();
    auto &vvi16StoredTimeData = m_TimeDataSourceMap[vu8SourceIdentifier];

    bool bChannelCountTheSame = true;
    if (vvi16StoredTimeData.size() != pTimeChunk->m_vvi16TimeChunks.size())
        bChannelCountTheSame = false;

    return bChannelCountTheSame;
}

void TimeChunkSynchronisationModule::TryInitialiseDataSource(std::shared_ptr<TimeChunk> pTimeChunk)
{
    const auto& vu8SourceId = pTimeChunk->GetSourceIdentifier();
    int64_t i64MostRecentTimeStamp = pTimeChunk->m_i64TimeStamp;

    if (!m_TimeDataSourceMap[vu8SourceId].empty())
        return;
    
    m_MostRecentSourceTimestamp[vu8SourceId] = i64MostRecentTimeStamp;

    auto stChannelCount = pTimeChunk->m_vvi16TimeChunks.size();
    m_TimeDataSourceMap[vu8SourceId].resize(stChannelCount);

    

}

void TimeChunkSynchronisationModule::StoreData(std::shared_ptr<TimeChunk> pTimeChunk)
{
    auto vu8SourceIdentifier = pTimeChunk->GetSourceIdentifier();
    auto &vi16NewTimeData = pTimeChunk->m_vvi16TimeChunks[0];

    m_MostRecentSourceTimestamp[vu8SourceIdentifier] = pTimeChunk->m_i64TimeStamp;
    m_dSampleRate_hz = pTimeChunk->m_dSampleRate;


    auto &vvi16StoredTimeData = m_TimeDataSourceMap[vu8SourceIdentifier];
    auto count = 0;
    for (auto& vi16Data :  pTimeChunk->m_vvi16TimeChunks) 
    {
        // TODO: check the if sub vec has been defined
        auto &vi16StoredData = vvi16StoredTimeData[count];
        vi16StoredData.insert(vi16StoredData.end(), vi16Data.begin(), vi16Data.end());
        count++;
    }
    
}