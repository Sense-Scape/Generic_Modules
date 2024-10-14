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

    // Store actual time data instead of TimeChunk
    m_TimeDataSourceMap[pTimeChunk->GetSourceIdentifier()].emplace_back(pTimeChunk->m_vvi16TimeChunks[0]);
    m_NewestSourceTimestampMap[pTimeChunk->GetSourceIdentifier()] = pTimeChunk->m_i64TimeStamp;
    m_u64SampleRate_hz = pTimeChunk->m_i64TimeStamp;

    auto tpNow = std::chrono::steady_clock::now();
    auto u64ElapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(tpNow - m_tpLastSyncAttempt).count();

    if (u64ElapsedNs >= m_u64SyncIntervalNs)
    {
        SynchronizeAndProcessChunks();
        m_tpLastSyncAttempt = tpNow;
    }
}

void TimeChunkSynchronisationModule::SynchronizeAndProcessChunks()
{
    if (m_TimeDataSourceMap.empty()) 
        return;

    bool bAllQueuesHaveData = true;
    uint64_t u64EarliestTimestamp = std::numeric_limits<uint64_t>::max();

    // Check if all queues have data and find the earliest timestamp
    for (const auto& queuePair : m_TimeDataSourceMap)
    {
        if (queuePair.second.empty())
        {
            bAllQueuesHaveData = false;
            break;
        }
        u64EarliestTimestamp = std::min(u64EarliestTimestamp, m_NewestSourceTimestampMap[queuePair.first] );
    }

    if (!bAllQueuesHaveData) 
        return;

    // Synchronize data by removing samples until timestamps match
    for (auto& queuePair : m_TimeDataSourceMap)
    {
        auto i64CurrentSourceTimeStamp = m_NewestSourceTimestampMap[queuePair.first];
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

    // // Process synchronized data
    // std::vector<std::pair<uint64_t, std::vector<float>>> vSynchronizedData;
    // for (auto& queuePair : m_TimeDataSourceMap)
    // {
    //     vSynchronizedData.push_back(queuePair.second.front());
    //     queuePair.second.pop();
    // }

    // TODO: Process the synchronized data as needed
    // For example, you could create a new chunk type that contains all synchronized data
    // and pass it to the next module using TryPassChunk()
}
