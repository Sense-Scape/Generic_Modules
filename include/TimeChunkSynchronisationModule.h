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
    TimeChunkSynchronisationModule(unsigned uBufferSize, uint64_t u64ThresholdNs, uint64_t u64SyncIntervalNs);
    ~TimeChunkSynchronisationModule() = default;

    std::string GetModuleType() override { return "TimeChunkSynchronisationModule"; }

private:
    void Process_TimeChunk(std::shared_ptr<BaseChunk> pBaseChunk);
    void SynchronizeAndProcessChunks();

    std::map<std::vector<uint8_t>, std::vector<int16_t>> m_TimeDataSourceMap;
    std::map<std::vector<uint8_t>, uint64_t> m_NewestSourceTimestampMap;
    uint64_t m_u64SampleRate_hz;
    uint64_t m_u64ThresholdNs;
    uint64_t m_u64SyncIntervalNs;
    std::chrono::steady_clock::time_point m_tpLastSyncAttempt;
};

#endif
