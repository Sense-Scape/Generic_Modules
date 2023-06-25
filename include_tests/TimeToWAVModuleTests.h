#ifndef TIME_TO_WAV_MODULE_TESTS
#define TIME_TO_WAV_MODULE_TESTS

#include "doctest.h"
#include "TimeToWAVModule.h"

TEST_CASE("TimeToWAVModule Test")
{
    unsigned uBufferSize = 10;
    TimeToWAVModule timeToWAVModule(uBufferSize);

    SUBCASE("Checking default constructor") {
        CHECK(timeToWAVModule.GetModuleType() == ModuleType::TimeToWavModule);
    }

    // Creating time chunk parameters
    double dChunkSize = 128;
    double dSampleRate = 44100;
    uint64_t i64TimeStamp = 1000000;
    unsigned uBits = 16;
    unsigned uNumBytes = int(uBits/8);
    unsigned uNumChannels = 2;
    // Creating data
    std::vector<std::vector<int16_t>> vvi16Data;
    vvi16Data.resize(uNumChannels);
    for (size_t i = 0; i < uNumChannels; i++)
        vvi16Data[i].resize(dChunkSize);

    auto pTimeChunk = std::make_shared<TimeChunk>(dChunkSize, dSampleRate, i64TimeStamp, uBits, uNumBytes, uNumChannels);
    pTimeChunk->m_vvi16TimeChunks = vvi16Data;

    SUBCASE("Checking processing of this module") {
        CHECK(timeToWAVModule.GetModuleType() == ModuleType::TimeToWavModule);
    }

    // Emulating processing
    timeToWAVModule.SetTestMode(true);
    timeToWAVModule.TestProcess(pTimeChunk);
    auto pBaseChunk = timeToWAVModule.GetTestOutput();
    auto pWAVChunk = std::static_pointer_cast<WAVChunk>(pBaseChunk);

    // Generating expected data
    std::vector<int16_t> vi16ExpectedData;
    vi16ExpectedData.resize(uNumChannels* dChunkSize);

    // Generating Expected results
    auto pExpectedWAVChunk = std::make_shared<WAVChunk>();
    pExpectedWAVChunk->m_i64TimeStamp = i64TimeStamp;
    pExpectedWAVChunk->m_vi16Data = vi16ExpectedData;
   

    SUBCASE("Checking processing output") {
        CHECK(pWAVChunk->GetChunkType() == ChunkType::WAVChunk);
        CHECK(pWAVChunk->m_i64TimeStamp == pExpectedWAVChunk->m_i64TimeStamp);
        CHECK(pWAVChunk->m_vi16Data == pExpectedWAVChunk->m_vi16Data);
    }
}

#endif