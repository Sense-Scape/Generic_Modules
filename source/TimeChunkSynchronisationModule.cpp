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

    // Store time data instead of only the first channel of data
    auto& vi16TimeData = m_TimeDataSourceMap[pTimeChunk->GetSourceIdentifier()];
    vi16TimeData.insert(vi16TimeData.end(), pTimeChunk->m_vvi16TimeChunks[0].begin(), pTimeChunk->m_vvi16TimeChunks[0].end());

    // Then store the timestamp and sample rate of data (assuming sample rate is constant for all data)
    m_MostRecentSourceTimestamp[pTimeChunk->GetSourceIdentifier()] = pTimeChunk->m_i64TimeStamp;
    m_u64SampleRate_hz = pTimeChunk->m_i64TimeStamp;

    CheckAndPerformSynchronization();
}

void TimeChunkSynchronisationModule::CheckAndPerformSynchronization()
{
    auto tpNow = std::chrono::steady_clock::now();
    auto u64ElapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(tpNow - m_tpLastSyncAttempt).count();

    if (u64ElapsedNs >= m_u64SyncIntervalNs)
    {
        SynchronizeAndProcessChunks();
        m_tpLastSyncAttempt = tpNow;
    }
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

void TimeChunkSynchronisationModule::SynchronizeAndProcessChunks()
{
    // Check if we have enough sources to do multilateration
    bool bEnoughSourcesToDoMultilateration = m_TimeDataSourceMap.size() >= 3;
    if (!bEnoughSourcesToDoMultilateration) 
        return;
        
    if (!CheckAllQueuesHaveData())
        return;
    

    // Now that we know we have sources, check if all queues have data and find the earliest timestamp
    bool bDiscontinuityDetected = false;
    uint64_t u64EarliestTimestamp = std::numeric_limits<uint64_t>::max();


    for (const auto& queuePair : m_TimeDataSourceMap)
    {
        
        
        uint64_t u64CurrentTimestamp = m_MostRecentSourceTimestamp[queuePair.first];
        u64EarliestTimestamp = std::min(u64EarliestTimestamp, u64CurrentTimestamp);

        // Check for discontinuity
        if (m_LastProcessedTimestampMap.find(queuePair.first) != m_LastProcessedTimestampMap.end())
        {
            uint64_t u64LastTimestamp = m_LastProcessedTimestampMap[queuePair.first];
            uint64_t u64TimeDifference = u64CurrentTimestamp - u64LastTimestamp;
            
            if (u64TimeDifference > m_u64ThresholdNs)
            {
                PLOG_WARNING << "Discontinuity detected for source: " << queuePair.first.data() 
                             << ". Time difference: " << u64TimeDifference << " ns";
                bDiscontinuityDetected = true;
                break;
            }
        }
        
        // Update the last processed timestamp
        m_LastProcessedTimestampMap[queuePair.first] = u64CurrentTimestamp;
    }

    if (!bAllQueuesHaveData || bDiscontinuityDetected) 
    {
        // If a discontinuity is detected, we might want to clear the buffers and start over
        if (bDiscontinuityDetected)
        {
            for (auto& queuePair : m_TimeDataSourceMap)
            {
                queuePair.second.clear();
            }
            m_LastProcessedTimestampMap.clear();
        }
        return;
    }

    // Synchronize data by removing samples until timestamps match
    for (auto& queuePair : m_TimeDataSourceMap)
    {
        auto i64CurrentSourceTimeStamp = m_MostRecentSourceTimestamp[queuePair.first];
        auto& vfDataVector = queuePair.second;

        if (!vfDataVector.empty() && i64CurrentSourceTimeStamp < u64EarliestTimestamp)
        {
            uint64_t timeDifference = u64EarliestTimestamp - i64CurrentSourceTimeStamp;
            size_t elementsToRemove = timeDifference / (1e9 / m_u64SampleRate_hz); // Assuming m_dSampleRate is in Hz

            if (elementsToRemove > 0)
            {
                if (elementsToRemove >= vfDataVector.size())
                {
                    PLOG_WARNING << "Dropping all samples from source: " << queuePair.first.data();
                    vfDataVector.clear();
                }
                else
                {
                    vfDataVector.erase(vfDataVector.begin(), vfDataVector.begin() + elementsToRemove);
                    PLOG_WARNING << "Dropping " << elementsToRemove << " samples from source: " << queuePair.first.data();
                }
            }
        }
    }

    // Check if all queues still have data after synchronization
    for (const auto& queuePair : m_TimeDataSourceMap)
    {
        if (queuePair.second.empty())
        {
            return; // Not enough synchronized data, wait for next cycle
        }
    }

    // Process synchronized data
    std::vector<std::pair<uint64_t, std::vector<float>>> vSynchronizedData;
    for (auto& queuePair : m_TimeDataSourceMap)
    {
        vSynchronizedData.push_back(queuePair.second.front());
        queuePair.second.pop();
    }

    // TODO: Process the synchronized data as needed
    // For example, you could create a new chunk type that contains all synchronized data
    // and pass it to the next module using TryPassChunk()
}
