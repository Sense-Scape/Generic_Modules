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

