#ifndef SIMULATOR_MODULE_TESTS
#define SIMULATOR_MODULE_TESTS

#include "doctest.h"
#include "SimulatorModule.h"

TEST_CASE("Simulator Module Test")
{
    unsigned uBufferSize = 10;
    double dSampleRate = 44100;
    double dChunkSize = 512;
    unsigned uNumChannels = 2;
    unsigned uSimulatedFrequency = 10000;
    std::vector<uint8_t> vuSourceIdentifier = {10, 10};

    SimulatorModule simulatorModule(dSampleRate, dChunkSize, uNumChannels, uSimulatedFrequency, vuSourceIdentifier, uBufferSize);


    CHECK(simulatorModule.GetModuleType() == ModuleType::SimulatorModule);

    // Set to test mode and create dummy chunk
    simulatorModule.SetTestMode(true);
    std::shared_ptr<BaseChunk> pBaseChunk = std::make_shared<BaseChunk>();

    // Test process this chunk
    simulatorModule.TestProcess(pBaseChunk);
    auto pBaseChunkOut0 = simulatorModule.GetTestOutput();
    auto pTimeChunkOut0 = std::static_pointer_cast<TimeChunk>(pBaseChunkOut0);

    simulatorModule.TestProcess(pBaseChunk);
    auto pBaseChunkOut1 = simulatorModule.GetTestOutput();
    auto pTimeChunkOut1 = std::static_pointer_cast<TimeChunk>(pBaseChunkOut1);
   


   SUBCASE("Checking simulation functionality") {

       // Then lets check first chunk
       int16_t u16FirstSampleFirstChunk = 0;
       CHECK(*pTimeChunkOut0->m_vvi16TimeChunks[0].begin() == u16FirstSampleFirstChunk);
       int16_t u16LastSampleFirstChunk = (std::pow(2, 15) * sin(2 * 3.14159 * ((float)uSimulatedFrequency * (dChunkSize - 1) / (float)dSampleRate)));
       CHECK(*pTimeChunkOut0->m_vvi16TimeChunks[0].end() == u16LastSampleFirstChunk);

       // And then the second
       int16_t u16FirstSampleSecondChunk = std::pow(2, 15) * sin((2 * 3.14159 * (float)uSimulatedFrequency * dChunkSize) / (float)dSampleRate);
       CHECK(*pTimeChunkOut1->m_vvi16TimeChunks[0].begin() == u16FirstSampleSecondChunk);
       int16_t u16LastSampleSecondChunk = (std::pow(2, 15) * sin((2 * 3.14159 * (float)uSimulatedFrequency * ((2 * dChunkSize) - 1)) / (float)dSampleRate));
       CHECK(*pTimeChunkOut1->m_vvi16TimeChunks[0].end() == u16LastSampleSecondChunk);
   }

}

#endif