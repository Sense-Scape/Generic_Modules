#ifndef TIME_TO_WAV_MODULE_TESTS
#define TIME_TO_WAV_MODULE_TESTS

#include "doctest.h"
#include "TimeToWAVModule.h"

TEST_CASE("TimeToWAVModule Test")
{
    unsigned uBufferSize = 10;
    TimeToWAVModule timeToWAVModule(uBufferSize);

    CHECK(timeToWAVModule.GetModuleType() == ModuleType::TimeToWavModule);
}

#endif