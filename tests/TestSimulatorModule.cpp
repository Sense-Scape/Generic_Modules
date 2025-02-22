#include <gtest/gtest.h>
#include "SimulatorModule.h"

class TestSimulatorModule : public ::testing::Test {
protected:
    // You can use SetUp and TearDown for initialization and cleanup.
    void SetUp() override {
            
    }

    void TearDown() override {
        
    }

    std::shared_ptr<SimulatorModule> pSimulatorModule;

};

// Test we have the generate timestamp offset as a function of number of samples and sample rate
TEST_F(TestSimulatorModule, testSimTimestampIncrement) {
    
    nlohmann::json DefaultJsonConfig = { 
        {"Enabled", "TRUE"},
        {"SampleRate_Hz", 16000},
        {"SimulatedFrequency_Hz", 500},
        {"NumberOfChannels", 1},
        {"ChannelPhases_deg", {0}},
        {"SourceIdentifier", {3, 4, 5}},
        {"StartupDelay_us", 0},
        {"SNR_db", 30},
        {"ADCMode", "Gaussian"},
        {"ClockMode", "Counter"},
        {"SignalPower_dBm", 25}
    };

    pSimulatorModule = std::make_shared<SimulatorModule>(10, DefaultJsonConfig);

    auto bResult = pSimulatorModule->m_u64CurrentTimeStamp_us == 0;
    EXPECT_EQ(bResult, true) << " Testing initialisation of timestamp in simulation mode";

    pSimulatorModule->GenerateCountTimestamp();
    bResult = pSimulatorModule->m_u64CurrentTimeStamp_us == uint64_t(1'000'000*(512.0/16000.0));
    EXPECT_EQ(bResult, true) << " Testing initialisation of timestamp in simulation mode";
}

// Test we can add an offset tot the start timestamp
TEST_F(TestSimulatorModule, testSimTimestampStartOffset) {

    nlohmann::json TimeOffsetJsonConfig = { 
        {"Enabled", "TRUE"},
        {"SampleRate_Hz", 16000},
        {"SimulatedFrequency_Hz", 500},
        {"NumberOfChannels", 1},
        {"ChannelPhases_deg", {0}},
        {"SourceIdentifier", {3, 4, 5}},
        {"StartupDelay_us", 29155},
        {"SNR_db", 30},
        {"ADCMode", "Gaussian"},
        {"ClockMode", "Counter"},
        {"SignalPower_dBm", 25}
    };

    pSimulatorModule = std::make_shared<SimulatorModule>(10, TimeOffsetJsonConfig);

    bool bResult = pSimulatorModule->m_u64CurrentTimeStamp_us == 29155;
    EXPECT_EQ(bResult, true) << " Testing initialisation of timestamp in simulation mode";

    // Test we have the offset
    pSimulatorModule->GenerateCountTimestamp();
    bResult = pSimulatorModule->m_u64CurrentTimeStamp_us == 29155 + uint64_t(1'000'000*(512.0/16000.0));
    EXPECT_EQ(bResult, true) << " Testing initialisation of timestamp in simulation mode";
}

