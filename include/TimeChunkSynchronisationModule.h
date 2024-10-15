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

    /**
     * @brief Check if we need to perform synchronization and perform it if necessary
     * @return True if we should perform synchronization, false otherwise
     */
    bool ShouldWeTrySynchronise();

    /**
     * @brief Check if all queues have data
     * @return True if all queues have data, false otherwise
     */
    bool CheckAllQueuesHaveData();

    /**
     * @brief Synchronize the channels of the time data
     */
    void SynchronizeChannels();

    std::map<std::vector<uint8_t>, std::vector<int16_t>> m_TimeDataSourceMap;

    uint64_t m_u64SampleRate_hz;
    uint64_t m_u64ThresholdNs;
    uint64_t m_u64SyncIntervalNs;
    std::chrono::steady_clock::time_point m_tpLastSyncAttempt;
    std::map<std::vector<uint8_t>, uint64_t> m_OldestSourceTimestampMap;  ///< Time stamps from the oldest chunk received from each source   
    std::map<std::vector<uint8_t>, uint64_t> m_MostRecentSourceTimestamp; ///< Time stamps from the most recent chunk received from each source
};

#endif
