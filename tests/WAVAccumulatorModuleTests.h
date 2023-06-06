#ifndef WAV_ACCUMULATOR_MODULE_TESTS
#define WAV_ACCUMULATOR_MODULE_TESTS

#include "doctest.h"
#include "WAVAccumulator.h"

TEST_CASE("WAVAccumulatorModule Test")
{
    unsigned uBufferSize = 10;
    SessionProcModule sessionProcModule(uBufferSize);

    CHECK(sessionProcModule.GetModuleType() == ModuleType::SessionProcModule);
}

#endif