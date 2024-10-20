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
    /**
     * @brief Constructor for TimeChunkSynchronisationModule
     * @param uBufferSize The size of the buffer for storing time chunks
     * @param u64Threshold_ns The threshold in nanoseconds for time synchronization
     * @param u64SyncInterval_ns The interval in nanoseconds between synchronization attempts
     */
    TimeChunkSynchronisationModule(unsigned uBufferSize, uint64_t u64Threshold_ns, uint64_t u64SyncInterval_ns);
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
    bool CheckQueuesHaveDataForMultilateration();

    /**
     * @brief Synchronize the channels of the time data
     */
    void SynchronizeChannels();

    /**
     * @brief Check if the new timestamp is continuous with the previous one
     * @param sourceId The identifier of the data source
     * @param i64NewTimestamp The new timestamp to check
     * @param uNumSamples The number of samples between the last timestamp and the new one
     * @param dSampleRate_hz The sample rate of the data source
     * @return True if the timestamp is continuous, false otherwise
     */
    bool IsTimestampContinuous(const std::vector<uint8_t>& sourceId, int64_t i64NewTimestamp, size_t uNumSamples, double dSampleRate_hz);

        /**
     * @brief Clears all internal state of the module
     */
    void ClearState();

    std::map<std::vector<uint8_t>, std::vector<int16_t>> m_TimeDataSourceMap;

    double m_dSampleRate_hz;
    uint64_t m_u64Threshold_ns;
    uint64_t m_u64SyncInterval_ns;
    std::chrono::steady_clock::time_point m_tpLastSyncAttempt;
    std::map<std::vector<uint8_t>, uint64_t> m_OldestSourceTimestampMap;  ///< Time stamps from the oldest chunk received from each source   
    std::map<std::vector<uint8_t>, uint64_t> m_MostRecentSourceTimestamp; ///< Time stamps from the most recent chunk received from each source

    /**
     * @brief Check if all queues have at least n seconds worth of data
     * @param dSecondsRequired The number of seconds worth of data required in each queue
     * @return True if all queues have enough data, false otherwise
     */
    bool CheckQueuesHaveEnoughData(double dSecondsRequired);

    /**
     * @brief Creates a new TimeChunk with synchronized data from all sources
     * @return A shared pointer to the new synchronized TimeChunk, or nullptr if no data is available
     */
    std::shared_ptr<TimeChunk> CreateSynchronizedTimeChunk();
};

#endif
