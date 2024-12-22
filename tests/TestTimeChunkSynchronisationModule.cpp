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
        
        pTimeChunkThreeChannel = std::make_shared<TimeChunk>(dChunkSize,dSampleRate,i64TimeStamp,uBits,uNumBytes,uNumChannels+1);
        pTimeChunkThreeChannel->SetSourceIdentifier({2});

        pTimeChunkFourChannel = std::make_shared<TimeChunk>(dChunkSize,dSampleRate,i64TimeStamp,uBits,uNumBytes,uNumChannels+2);
        pTimeChunkFourChannel->SetSourceIdentifier({3});
    }

    void TearDown() override {
        
    }

    std::shared_ptr<TimeChunkSynchronisationModule> pTimeChunkSynchronisationModule;
    std::shared_ptr<TimeChunk> pTimeChunkTwoChannel;
    std::shared_ptr<TimeChunk> pTimeChunkThreeChannel;
    std::shared_ptr<TimeChunk> pTimeChunkFourChannel;
};

TEST_F(TestTimeSyncClass, TestTrackingChannelCounts) {

    auto bResult = pTimeChunkSynchronisationModule->IsChannelCountTheSame(pTimeChunkTwoChannel);
    EXPECT_EQ(bResult, false) << " Testing initialisation channel count";
    
    pTimeChunkSynchronisationModule->TryInitialiseDataSource(pTimeChunkTwoChannel);
    bResult = pTimeChunkSynchronisationModule->IsChannelCountTheSame(pTimeChunkTwoChannel);
    EXPECT_EQ(bResult, true) << " Testing channel count comparison is correct";

    bResult = pTimeChunkSynchronisationModule->IsChannelCountTheSame(pTimeChunkThreeChannel);
    EXPECT_EQ(bResult, false) << " Testing channel count comparison fails with different number of channels";;

}

TEST_F(TestTimeSyncClass, TestMultiLaterationSourceCount) {

    pTimeChunkSynchronisationModule->Process_TimeChunk(pTimeChunkTwoChannel);
    pTimeChunkSynchronisationModule->Process_TimeChunk(pTimeChunkThreeChannel);
    bool bResult = pTimeChunkSynchronisationModule->CheckQueuesHaveDataForMultilateration();
    EXPECT_EQ(bResult, false) << " Testing we fail multilateration check with 2 channels";

    pTimeChunkSynchronisationModule->Process_TimeChunk(pTimeChunkFourChannel);
    bResult = pTimeChunkSynchronisationModule->CheckQueuesHaveDataForMultilateration();
    EXPECT_EQ(bResult, true) << " Testing we pass multilateration check with 3 channels";

}
