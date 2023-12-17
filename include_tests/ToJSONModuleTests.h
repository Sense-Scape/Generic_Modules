#ifndef TO_JSON_MODULE_TESTS
#define TO_JSON_MODULE_TESTS

#include "doctest.h"
#include "ToJSONModule.h"
#include "json.hpp"

TEST_CASE("ToJSONModule Test")
{
    unsigned uBufferSize = 10;

    ToJSONModule toJSONModule;
    toJSONModule.SetTestMode(true);


    CHECK(toJSONModule.GetModuleType() == ModuleType::ToJSONModule);

 
    // Constructor Parameters
    double dChunkSize = 512;
    double dSampleRate = 44100;
    uint64_t i64TimeStamp = 100000;
    unsigned uBits = 16;
    unsigned uNumBytes = 2;
    unsigned uNumChannels = 2;

    std::vector<int16_t> vu16ChannelOne;
    vu16ChannelOne.assign(dChunkSize, 1);
    std::vector<int16_t> vu16ChannelTwo;
    vu16ChannelTwo.assign(dChunkSize, 2);
    // All the above sum to bytes below - size of class
    BaseChunk baseChunk;
    // Size of header info, size of channels and size of base class
    unsigned uClassSize_bytes = 36 + dChunkSize * uNumChannels * uNumBytes + baseChunk.GetSize();
    // Lets just start by creating a Timechunk
    std::shared_ptr<TimeChunk>  pTimeChunkTestClass = std::make_shared<TimeChunk>(dChunkSize, dSampleRate, i64TimeStamp, uBits, uNumBytes, uNumChannels);
    pTimeChunkTestClass->m_vvi16TimeChunks[0] = vu16ChannelOne;
    pTimeChunkTestClass->m_vvi16TimeChunks[1] = vu16ChannelTwo;

    // Creating reference data
    auto JSONDocument = nlohmann::json();
    auto strChunkName = ChunkTypesNamingUtility::toString(ChunkType::TimeChunk);
    JSONDocument[strChunkName]["SourceIndentifierSize"] = std::to_string(0);
    JSONDocument[strChunkName]["SourceIndentifier"] = std::vector<uint8_t>();
    JSONDocument[strChunkName]["ChunkSize"] = std::to_string(dChunkSize);
    JSONDocument[strChunkName]["SampleRate"] = std::to_string(dSampleRate);
    JSONDocument[strChunkName]["TimeStamp"] = std::to_string(i64TimeStamp);
    JSONDocument[strChunkName]["uBits"] = std::to_string(uBits);
    JSONDocument[strChunkName]["NumBytes"] = std::to_string(uNumBytes);
    JSONDocument[strChunkName]["NumChannels"] = std::to_string(uNumChannels);
    JSONDocument[strChunkName]["Channels"][std::to_string(0)] = vu16ChannelOne;
    JSONDocument[strChunkName]["Channels"][std::to_string(1)] = vu16ChannelTwo;

    // Test Processing
    toJSONModule.TestProcess(pTimeChunkTestClass);
    auto pBaseChunkOut = toJSONModule.GetTestOutput();
    auto pJSONChunkOut = std::static_pointer_cast<JSONChunk>(pBaseChunkOut);

    // Lets also check what happens if the class does not have the ToJSON conversion
    std::shared_ptr<BaseChunk>  pBaseChunkTestClass = std::make_shared<BaseChunk>();
    toJSONModule.TestProcess(pBaseChunkTestClass);
    //auto pBaseChunkOut2 = toJSONModule.GetTestOutput();

    SUBCASE("Checking ToJSON Converter") {
        // Assuming ToJSON implemented, check that processing is correct
        /*CHECK(pJSONChunkOut->m_JSONDocument == JSONDocument);*/
        // Check that no processing takes place if the chunk does not have ToJSON implemented
        // CHECK(pBaseChunkOut2 == nullptr);
    }

}

#endif