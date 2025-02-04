#include "TimeChunkSynchronisationModule.h"
#include <algorithm>
#include <limits>
#include <chrono>

TimeChunkSynchronisationModule::TimeChunkSynchronisationModule(unsigned uBufferSize, uint64_t u64Threshold_us, uint64_t u64SyncInterval_ns)
    : BaseModule(uBufferSize), m_u64ChannelDiscontinuityThreshold_us(u64Threshold_us), m_u64SyncInterval_ns(u64SyncInterval_ns),
      m_tpLastSyncAttempt(std::chrono::steady_clock::now())
{
    RegisterChunkCallbackFunction(ChunkType::TimeChunk, &TimeChunkSynchronisationModule::Process_TimeChunk, (BaseModule*)this);
    RegisterChunkCallbackFunction(ChunkType::GPSChunk, &TimeChunkSynchronisationModule::Process_GPSChunk, (BaseModule*)this);
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

void TimeChunkSynchronisationModule::Process_GPSChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    auto pGPSChunk  = std::static_pointer_cast<GPSChunk>(pBaseChunk);
    auto vu8SourceIdentifier = pGPSChunk->GetSourceIdentifier();

    auto dLongScaling = pGPSChunk->m_bIsWest ? -1 : 1;
    auto dLatScaling = pGPSChunk->m_bIsNorth ? 1 : -1;

    auto dLong = pGPSChunk->m_dLongitude;
    auto dLat = pGPSChunk->m_dLatitude;

    m_dSourceLongitudesMap[vu8SourceIdentifier] = dLong;
    m_dSourceLatitudesMap[vu8SourceIdentifier] = dLat;

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

    if (!CheckWeHaveEnoughGPSData())
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
        if (queuePair.second[0].size() <= uSamplesRequired)
            return false;
    }
    
    return true;
}

bool TimeChunkSynchronisationModule::CheckWeHaveEnoughGPSData()
{
    if (m_dSourceLongitudesMap.empty() || m_dSourceLatitudesMap.empty())
        return false;
        
    // Just a rudimentary check to start
    bool bChannelCountSameAsLong = m_OldestSourceTimestampMap.size() == m_dSourceLongitudesMap.size();
    bool bChannelCountSameAsLat = m_OldestSourceTimestampMap.size() == m_dSourceLatitudesMap.size();

    bool bEnoughGPSData = bChannelCountSameAsLong && bChannelCountSameAsLat;

    return bEnoughGPSData;
}

void TimeChunkSynchronisationModule::SynchronizeChannels()
{   
    // Find the most recent timestamp from all the channels
    uint64_t u64MostRecentStaterTimestamp = 0;
    for (const auto& pair : m_OldestSourceTimestampMap)
        u64MostRecentStaterTimestamp = std::max(u64MostRecentStaterTimestamp, pair.second);

    // Calculate and remove excess samples from each channel
    for (auto& pair : m_TimeDataSourceMap)
    {
        const auto& vu8SourceId = pair.first;
        auto& vvi16TimeData = pair.second;

        int64_t i64TimeDifference = u64MostRecentStaterTimestamp - m_OldestSourceTimestampMap[vu8SourceId];
        auto u32SamplesToRemove = (uint32_t)(m_dSampleRate_hz*i64TimeDifference/ 1e6);

        for (auto &vi16TimeData : vvi16TimeData)
        {
            if (u32SamplesToRemove > 0 && u32SamplesToRemove < vi16TimeData.size())
            {
                m_OldestSourceTimestampMap[vu8SourceId] = u64MostRecentStaterTimestamp;
                vi16TimeData.erase(vi16TimeData.begin(), vi16TimeData.begin() + u32SamplesToRemove);
            }
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

    m_dSourceLatitudesMap.clear();
    m_dSourceLongitudesMap.clear();
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
        m_OldestSourceTimestampMap.begin()->second,
        numSamples
    );


    pTDOAChunk->SetSourceIdentifier({1,1,1});

    // Fill the TimeChunk with synchronized data
    for (auto& [vu8SourceId, vvi16SourceData] : m_TimeDataSourceMap)
    {

        std::vector<std::vector<int16_t>> vvu16TmpVec;
        vvu16TmpVec.resize(vvi16SourceData.size());

        for (size_t i = 0; i < vvi16SourceData.size(); i++)
        {
            vvu16TmpVec[i] = std::vector(vvi16SourceData[i].begin(), vvi16SourceData[i].begin()+numSamples);
            vvi16SourceData[i].erase(vvi16SourceData[i].begin(), vvi16SourceData[i].begin()+numSamples);
        }
        

        pTDOAChunk->AddData(
                        m_dSourceLongitudesMap[vu8SourceId],
                        m_dSourceLatitudesMap[vu8SourceId],
                        vu8SourceId, 
                        vvu16TmpVec);
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

    m_MostRecentSourceTimestamp[vu8SourceIdentifier] = pTimeChunk->m_i64TimeStamp;
    m_dSampleRate_hz = pTimeChunk->m_dSampleRate;

    auto count = 0;
    for (auto vi16Data :  pTimeChunk->m_vvi16TimeChunks) 
    {
        m_TimeDataSourceMap[vu8SourceIdentifier][count].insert( 
                                                            m_TimeDataSourceMap[vu8SourceIdentifier][count].end(), 
                                                            vi16Data.begin(), 
                                                            vi16Data.end());
        count++;
    }
    
}