#ifndef FFT_MODULE_TESTS
#define FFT_MODULE_TESTS

#include "doctest.h"
#include "FFTModule.h"

TEST_CASE("FFTModule Test")
{
    unsigned uBufferSize = 10;

    FFTModule ffModule(uBufferSize);
    //ffModule.SetTestMode(true);
    //
    //
    //CHECK(ffModule.GetModuleType() == ModuleType::FFTModule);


}

#endif