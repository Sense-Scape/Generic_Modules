#ifndef FFT_MODULE_TESTS
#define FFT_MODULE_TESTS

#include "doctest.h"
#include "FFTModule.h"

TEST_CASE("FFTModule Test")
{
    unsigned uBufferSize = 10;
    FFTModule fftModule(uBufferSize);
    fftModule.SetTestMode(true);

    SUBCASE("FFTModule constructor test")
    {
        CHECK(fftModule.GetModuleType() == "FFTModule");
    }

    // Creating time chunk parameters
    double dChunkSize = 128;
    double dSampleRate = 44100;
    uint64_t i64TimeStamp = 1000000;
    unsigned uBits = 16;
    unsigned uNumBytes = int(uBits / 8);
    unsigned uNumChannels = 2;

    unsigned uSimulatedFrequencyIndex = 10;
    unsigned uSimulatedFrequency = uSimulatedFrequencyIndex * 44100 / dChunkSize;

    // Creating data
    std::vector<std::vector<int16_t>> vvi16Data;
    vvi16Data.resize(uNumChannels);
    for (size_t i = 0; i < uNumChannels; i++)
        vvi16Data[i].resize(dChunkSize);
    // First channel gets DC data
    std::fill(vvi16Data[0].begin(), vvi16Data[0].end(), 1);
    // Second channel gets Sinusoid data
    for (unsigned uSampleIndex = 0; uSampleIndex < dChunkSize; uSampleIndex++)
        vvi16Data[1][uSampleIndex] = (std::pow(2, 15) - 1) * sin((double)2.0 * 3.141592653589793238462643383279502884197 * ((double)uSimulatedFrequency * (double)(uSampleIndex) / (double)dSampleRate));

    // Lets now create a time chunk
    auto pTimeChunk = std::make_shared<TimeChunk>(dChunkSize, dSampleRate, i64TimeStamp, uBits, uNumBytes, uNumChannels);
    pTimeChunk->m_vvi16TimeChunks = vvi16Data;

    // And then try take an fft of it
    fftModule.TestProcess(pTimeChunk);
    auto pBaseChunk = fftModule.GetTestOutput();
    auto pFFTChunk = std::static_pointer_cast<FFTChunk>(pBaseChunk);

    // Use std::max_element to find the iterator to the maximum element for DC and sinusoid
    auto pDCPowerFFTArray = pFFTChunk->GetChannelPower(0);
    auto itrMaxDCElementPtr = std::max_element(pDCPowerFFTArray->begin(), pDCPowerFFTArray->end());
    unsigned uMaxDCIndex = std::distance(pDCPowerFFTArray->begin(), itrMaxDCElementPtr);

    auto pSinusoidalPowerFFTArray = pFFTChunk->GetChannelPower(1);
    auto itrMaxSinusoidalElementPtr = std::max_element(pSinusoidalPowerFFTArray->begin(), pSinusoidalPowerFFTArray->end());
    unsigned uMaxSinusoidalIndex = std::distance(pSinusoidalPowerFFTArray->begin(), itrMaxSinusoidalElementPtr);

    SUBCASE("Checking FFT logic")
    {
        CHECK(uMaxDCIndex == 0);
        CHECK(uMaxSinusoidalIndex == uSimulatedFrequencyIndex);
    }
}

#endif