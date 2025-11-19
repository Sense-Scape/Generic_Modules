#include <gtest/gtest.h>
#include "TimeChunkSynchronisationModule.h"

class TestTimeSyncClass : public ::testing::Test {
protected:
    // You can use SetUp and TearDown for initialization and cleanup.
    void SetUp() override {

        // ============================
        //          TimeSync
        // ============================
        pTimeChunkSynchronisationModule = std::make_shared<TimeChunkSynchronisationModule>(10,10,10);


        // ============================
        //          TimeChunk
        // ============================
        double dChunkSize = 512;
        double dSampleRate = 16000;
        uint64_t i64TimeStamp = 0;
        unsigned uBits = 16; 
        unsigned uNumBytes = 2;
        unsigned uNumChannels = 2;

        pTimeChunkTwoChannel = std::make_shared<TimeChunk>(dChunkSize,dSampleRate,i64TimeStamp,uBits,uNumBytes,uNumChannels);
        pTimeChunkTwoChannel->SetSourceIdentifier({1});
        pGPSChunkTwoChannel = std::make_shared<GPSChunk>(i64TimeStamp, 0, 0, true);
        pGPSChunkTwoChannel->SetSourceIdentifier({1});

        pTimeChunkThreeChannel = std::make_shared<TimeChunk>(dChunkSize,dSampleRate,i64TimeStamp,uBits,uNumBytes,uNumChannels+1);
        pTimeChunkThreeChannel->SetSourceIdentifier({2});
        pGPSChunkThreeChannel = std::make_shared<GPSChunk>(i64TimeStamp, 0, 0, true);
        pGPSChunkThreeChannel->SetSourceIdentifier({2});

        pTimeChunkFourChannel = std::make_shared<TimeChunk>(dChunkSize,dSampleRate,i64TimeStamp,uBits,uNumBytes,uNumChannels+2);
        pTimeChunkFourChannel->SetSourceIdentifier({3});
        pGPSChunkFourChannel = std::make_shared<GPSChunk>(i64TimeStamp, 0, 0, true);
        pGPSChunkFourChannel->SetSourceIdentifier({3});
    }

    void TearDown() override {
        
    }

    std::shared_ptr<TimeChunkSynchronisationModule> pTimeChunkSynchronisationModule;
    std::shared_ptr<TimeChunk> pTimeChunkTwoChannel;
    std::shared_ptr<GPSChunk> pGPSChunkTwoChannel;
    std::shared_ptr<TimeChunk> pTimeChunkThreeChannel;
    std::shared_ptr<GPSChunk> pGPSChunkThreeChannel;
    std::shared_ptr<TimeChunk> pTimeChunkFourChannel;
    std::shared_ptr<GPSChunk> pGPSChunkFourChannel;
};

// On reboot of a sensor we may change channel count, cmake sure this does not cause issues
TEST_F(TestTimeSyncClass, TestChannelCountsFromASourceRemainConstant) {

    auto bResult = pTimeChunkSynchronisationModule->IsChannelCountTheSame(pTimeChunkTwoChannel);
    EXPECT_EQ(bResult, false) << " Testing initialisation channel count";
    
    pTimeChunkSynchronisationModule->TryInitialiseDataSource(pTimeChunkTwoChannel);
    bResult = pTimeChunkSynchronisationModule->IsChannelCountTheSame(pTimeChunkTwoChannel);
    EXPECT_EQ(bResult, true) << " Testing channel count comparison is correct";

    bResult = pTimeChunkSynchronisationModule->IsChannelCountTheSame(pTimeChunkThreeChannel);
    EXPECT_EQ(bResult, false) << " Testing channel count comparison fails with different number of channels";;

}

// Ensure we have stored at least 3 souces of data  for multlateration
TEST_F(TestTimeSyncClass, TestWeHaveEnoughSourcesForMultlateration) {

    pTimeChunkSynchronisationModule->CallChunkCallbackFunction(pTimeChunkTwoChannel);
    pTimeChunkSynchronisationModule->CallChunkCallbackFunction(pTimeChunkThreeChannel);
    bool bResult = pTimeChunkSynchronisationModule->CheckQueuesHaveDataForMultilateration();
    EXPECT_EQ(bResult, false) << " Testing we fail multilateration check with 2 sources";

    pTimeChunkSynchronisationModule->CallChunkCallbackFunction(pTimeChunkFourChannel);
    bResult = pTimeChunkSynchronisationModule->CheckQueuesHaveDataForMultilateration();
    EXPECT_EQ(bResult, true) << " Testing we pass multilateration check with 3 sources";

}

// There is no gaurentee of getting GPS and time data, if we have anough source data check
// if we have received GPS data from the corresponding source
TEST_F(TestTimeSyncClass, TestGPSWithTimeData) {

    bool bResult = pTimeChunkSynchronisationModule->CheckWeHaveEnoughGPSData();
    EXPECT_EQ(bResult, false) << " Testing we fail GPS check with no time no GPS sources";

    pTimeChunkSynchronisationModule->CallChunkCallbackFunction(pTimeChunkTwoChannel);
    bResult = pTimeChunkSynchronisationModule->CheckWeHaveEnoughGPSData();
    EXPECT_EQ(bResult, false) << " Testing we fail GPS check with 1 time no GPS sources";

    pTimeChunkSynchronisationModule->CallChunkCallbackFunction(pGPSChunkTwoChannel);
    bResult = pTimeChunkSynchronisationModule->CheckWeHaveEnoughGPSData();
    EXPECT_EQ(bResult, true) << " Testing we fail GPS check with 1 time and GPS sources";

    pTimeChunkSynchronisationModule->CallChunkCallbackFunction(pTimeChunkThreeChannel);
    bResult = pTimeChunkSynchronisationModule->CheckWeHaveEnoughGPSData();
    EXPECT_EQ(bResult, false) << " Testing we fail GPS check with 2 time no GPS sources";

    pTimeChunkSynchronisationModule->CallChunkCallbackFunction(pGPSChunkThreeChannel);
    bResult = pTimeChunkSynchronisationModule->CheckWeHaveEnoughGPSData();
    EXPECT_EQ(bResult, true) << " Testing we fail GPS check with 2 time and GPS sources";

    pTimeChunkSynchronisationModule->ClearState();
    bResult = pTimeChunkSynchronisationModule->CheckWeHaveEnoughGPSData();
    EXPECT_EQ(bResult, false) << " Testing we fail GPS check with 2 time and GPS sources";

    pTimeChunkSynchronisationModule->CallChunkCallbackFunction(pTimeChunkFourChannel);
    bResult = pTimeChunkSynchronisationModule->CheckWeHaveEnoughGPSData();
    EXPECT_EQ(bResult, false) << " Testing we fail GPS check with 2 time and GPS sources";

    pTimeChunkSynchronisationModule->CallChunkCallbackFunction(pGPSChunkFourChannel);
    bResult = pTimeChunkSynchronisationModule->CheckWeHaveEnoughGPSData();
    EXPECT_EQ(bResult, true) << " Testing we fail GPS check with 2 time and GPS sources";

}
