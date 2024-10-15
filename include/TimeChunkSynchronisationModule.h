#ifndef TIME_CHUNK_SYNCHRONISATION_MODULE
#define TIME_CHUNK_SYNCHRONISATION_MODULE

#include <map>
#include <queue>
#include <memory>
#include <vector>
#include <chrono>
#include "BaseModule.h"
#include "TimeChunk.h"

class TimeChunkSynchronisationModule : public BaseModule
{
public:
    TimeChunkSynchronisationModule(unsigned uBufferSize, uint64_t u64ThresholdNs, uint64_t u64SyncIntervalN);
    ~TimeChunkSynchronisationModule() = default;

    std::string GetModuleType() override { return "TimeChunkSynchronisationModule"; }

private:
    void Process_TimeChunk(std::shared_ptr<BaseChunk> pBaseChunk);
    void SynchronizeAndProcessChunks();

    /**
     * @brief Check if we need to perform synchronization and perform it if necessary
     */
    void CheckAndPerformSynchronization();

    /**
     * @brief Check if all queues have data
     * @return True if all queues have data, false otherwise
     */
    bool CheckAllQueuesHaveData();

    std::map<std::vector<uint8_t>, std::vector<int16_t>> m_TimeDataSourceMap;
    std::map<std::vector<uint8_t>, uint64_t> m_MostRecentSourceTimestamp;
    uint64_t m_u64SampleRate_hz;
    uint64_t m_u64ThresholdNs;
    uint64_t m_u64SyncIntervalNs;
    std::chrono::steady_clock::time_point m_tpLastSyncAttempt;
    std::unordered_map<std::string, uint64_t> m_LastProcessedTimestampMap;
};

#endif
