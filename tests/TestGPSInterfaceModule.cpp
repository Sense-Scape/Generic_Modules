#include <gtest/gtest.h>
#include "GPSInterfaceModule.h"

class TestGPSInterfaceClass : public ::testing::Test {
protected:
    // You can use SetUp and TearDown for initialization and cleanup.
    void SetUp() override {
            
    }

    void TearDown() override {
        
    }

    std::shared_ptr<GPSInterfaceModule> pGPSInterfaceModule;

};

TEST_F(TestGPSInterfaceClass, TestLongLatSetup) {

    nlohmann::json DefaultJsonConfig = {
                {"Enabled", "TRUE"},
                {"SimulatePosition", "TRUE"},
                {"SourceIdentifier", {1, 2, 3}},
                {"SerialPort", "/dev/ttyACM0"},
                {"SimulatedLongitude", 0},
                {"SimulatedLatitude", 0}
            };

    pGPSInterfaceModule = std::make_shared<GPSInterfaceModule>(10,DefaultJsonConfig);
    auto bResult = pGPSInterfaceModule->m_dSimulatedLongitude == 0;
    EXPECT_EQ(bResult, true) << " Testing initialisation longitude";
    bResult = pGPSInterfaceModule->m_dSimulatedLatitude == 0;
    EXPECT_EQ(bResult, true) << " Testing initialisation latitude";

    DefaultJsonConfig["SimulatedLongitude"] = 10;
    DefaultJsonConfig["SimulatedLatitude"] = 10;

    pGPSInterfaceModule = std::make_shared<GPSInterfaceModule>(10,DefaultJsonConfig);
    bResult = pGPSInterfaceModule->m_dSimulatedLongitude == 10;
    EXPECT_EQ(bResult, true) << " Testing initialisation lat of 10 deg";
    EXPECT_EQ(pGPSInterfaceModule->m_bSimulatedIsNorth, true)  << " Testing initialisation lat of 10 deg";
    bResult = pGPSInterfaceModule->m_dSimulatedLatitude == 10;
    EXPECT_EQ(bResult, true) << "  Testing initialisation long of 10 deg";
    EXPECT_EQ(pGPSInterfaceModule->m_bSimulatedIsWest, false)  << " Testing initialisation long of 10 deg";

    DefaultJsonConfig["SimulatedLongitude"] = -10;
    DefaultJsonConfig["SimulatedLatitude"] = -10;

    pGPSInterfaceModule = std::make_shared<GPSInterfaceModule>(10,DefaultJsonConfig);
    bResult = pGPSInterfaceModule->m_dSimulatedLongitude == 10;
    EXPECT_EQ(bResult, true) << " Testing initialisation lat of 10 deg";
    EXPECT_EQ(pGPSInterfaceModule->m_bSimulatedIsNorth, false)  << " Testing initialisation lat of -10 deg";
    bResult = pGPSInterfaceModule->m_dSimulatedLatitude == 10;
    EXPECT_EQ(bResult, true) << "  Testing initialisation long of 10 deg";
    EXPECT_EQ(pGPSInterfaceModule->m_bSimulatedIsWest, true)  << " Testing initialisation long of -10 deg";

    DefaultJsonConfig["SimulatedLongitude"] = 10;
    DefaultJsonConfig["SimulatedLatitude"] = -10;

    pGPSInterfaceModule = std::make_shared<GPSInterfaceModule>(10,DefaultJsonConfig);
    bResult = pGPSInterfaceModule->m_dSimulatedLongitude == 10;
    EXPECT_EQ(bResult, true) << " Testing initialisation lat of 10 deg";
    EXPECT_EQ(pGPSInterfaceModule->m_bSimulatedIsNorth, false)  << " Testing initialisation lat of -10 deg";
    bResult = pGPSInterfaceModule->m_dSimulatedLatitude == 10;
    EXPECT_EQ(bResult, true) << "  Testing initialisation long of 10 deg";
    EXPECT_EQ(pGPSInterfaceModule->m_bSimulatedIsWest, false)  << " Testing initialisation long of -10 deg";

    DefaultJsonConfig["SimulatedLongitude"] = -10;
    DefaultJsonConfig["SimulatedLatitude"] = 10;

    pGPSInterfaceModule = std::make_shared<GPSInterfaceModule>(10,DefaultJsonConfig);
    bResult = pGPSInterfaceModule->m_dSimulatedLongitude == 10;
    EXPECT_EQ(bResult, true) << " Testing initialisation lat of 10 deg";
    EXPECT_EQ(pGPSInterfaceModule->m_bSimulatedIsNorth, true)  << " Testing initialisation lat of -10 deg";
    bResult = pGPSInterfaceModule->m_dSimulatedLatitude == 10;
    EXPECT_EQ(bResult, true) << "  Testing initialisation long of 10 deg";
    EXPECT_EQ(pGPSInterfaceModule->m_bSimulatedIsWest, true)  << " Testing initialisation long of -10 deg";
}

