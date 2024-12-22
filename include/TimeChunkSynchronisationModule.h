#ifndef TIME_CHUNK_SYNCHRONISATION_MODULE
#define TIME_CHUNK_SYNCHRONISATION_MODULE

#include <map>
#include <queue>
#include <memory>
#include <vector>
#include <chrono>
#include "BaseModule.h"
#include "TimeChunk.h"
#include "TDOAChunk.h"

class TimeChunkSynchronisationModule : public BaseModule
{
public:
    /**
     * @brief Constructor for TimeChunkSynchronisationModule
     * @param uBufferSize The size of the buffer for storing time chunks
     * @param u64Threshold_ns The threshold in nanoseconds for time synchronization
     * @param u64SyncInterval_ns The interval in nanoseconds between synchronization attempts
     */
    TimeChunkSynchronisationModule(unsigned uBufferSize, uint64_t u64Threshold_us, uint64_t u64SyncInterval_ns);
    ~TimeChunkSynchronisationModule() = default;

    std::string GetModuleType() override { return "TimeChunkSynchronisationModule"; }

    /**
     * @brief Process store and transmit TDOA data
     * @param[in] pTimeChunk time chunk which to process
     */
    void Process_TimeChunk(std::shared_ptr<BaseChunk> pBaseChunk);

    /**
     * @brief Checking if data from a source has the same number of channels
     * @param[in] pTimeChunk time chunk which to process
     */
    bool IsChannelCountTheSame(std::shared_ptr<TimeChunk> pTimeChunk);

    /**
     * @brief Store initial timestamps and initialise channel counts without filling data
     * @param[in] pTimeChunk time chunk which to process
     */
    void TryInitialiseDataSource(std::shared_ptr<TimeChunk> pTimeChunk);

    /**
     * @brief Check if all queues have data
     * @return True if all queues have data, false otherwise
     */
    bool CheckQueuesHaveDataForMultilateration();

    /**
     * @brief Check if all queues have at least n seconds worth of data
     * @return True if all queues have enough data, false otherwise
     */
    bool CheckQueuesHaveEnoughData();


private:

    double m_dSampleRate_hz;                     ///< last recorded sample rate
    uint64_t m_u64ChannelTimoutThreshold_us;     ///< how long a channel cannot send data before a state clearance occurs
    uint64_t m_u64SyncInterval_ns;               ///< How long to wait before trying to synchronise channels
    uint64_t m_u64ChannelDiscontinuityThreshold_us;
    float m_TDOALength_s = 10;
    std::chrono::steady_clock::time_point m_tpLastSyncAttempt;                  ///< Time stamp of last synchronisation
    std::map<std::vector<uint8_t>, uint64_t> m_OldestSourceTimestampMap;        ///< Time stamps from the oldest chunk received from each source   
    std::map<std::vector<uint8_t>, uint64_t> m_MostRecentSourceTimestamp;       ///< Time stamps from the most recent chunk received from each source
    std::map<std::vector<uint8_t>, std::vector<std::vector<int16_t>>> m_TimeDataSourceMap;   ///< Map whioch stores each sources current time data

    /**
     * @brief Check if we need to perform synchronization and perform it if necessary
     * @return True if we should perform synchronization, false otherwise
     */
    bool ShouldWeTrySynchronise();

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
    bool IsDataContinuous(const std::vector<uint8_t>& sourceId, int64_t i64NewTimestamp, size_t uNumSamples, double dSampleRate_hz);

    
    /**
     * @brief Clears all internal state of the module
     */
    void ClearState();

    /**
     * @brief Creates a new TimeChunk with synchronized data from all sources
     * @return A shared pointer to the new synchronized TimeChunk, or nullptr if no data is available
     */
    std::shared_ptr<TDOAChunk> CreateSynchronizedTimeChunk();
    
    /**
     * @brief Checks if any of the channels has not received data for a specified amount of time
     */
    bool HasChannelTimeoutOccured();

    /**
     * @brief Stores time data in an internal map\
     * @param[in] pTimeChunk pointer to time chunk of data
     */
    void StoreData(std::shared_ptr<TimeChunk> pTimeChunk);
};

#endif
