#ifndef WAV_ACCUMULATOR_MODULE_TESTS
#define WAV_ACCUMULATOR_MODULE_TESTS

#include "doctest.h"
#include "WAVAccumulator.h"

TEST_CASE("WAVAccumulatorModule Test")
{
    unsigned uMaxInputBufferSize = 10;
    double dAccumulatePeriod = 10;
    double dContinuityThresholdFactor = 0.001;

    WAVAccumulator WAVAccumulator(dAccumulatePeriod, dContinuityThresholdFactor, uMaxInputBufferSize);

    CHECK(WAVAccumulator.GetModuleType() == ModuleType::WAVAccumulatorModule);
}

#endif