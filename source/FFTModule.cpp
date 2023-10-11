#include "FFTModule.h"

FFTModule::FFTModule(unsigned uBufferSize) : BaseModule(uBufferSize)
{

}

void FFTModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    auto pTimeChunk = std::static_pointer_cast<TimeChunk>(pBaseChunk);
    
    int iFFTPoints = pTimeChunk->m_dChunkSize;
    auto pFFTChunk = std::make_shared<FFTChunk>();

    // Forward FFT configuration
    kiss_fft_cfg ForwardFFTConfig = kiss_fft_alloc(iFFTPoints, 0, NULL, NULL);

    // Convert to complex vector
    std::vector<std::complex<float>> vcForwardFFTOutput(iFFTPoints, 0.0);
    pFFTChunk->m_vvcfFFTChunks.resize(pTimeChunk->m_uNumChannels);

    // Iterate through data channel
    for (uint16_t uChannelIndex = 0; uChannelIndex  < pTimeChunk->m_uNumChannels; uChannelIndex++)
    {
        // Store time data in FFTChunk 
        for (auto i16SubChunkSample : pTimeChunk->m_vvi16TimeChunks[uChannelIndex])
            // Creating a complex value with only real component
            pFFTChunk->m_vvcfFFTChunks[uChannelIndex].push_back(std::complex<float>(i16SubChunkSample, 0.0));
        // And then take FFT
        kiss_fft(ForwardFFTConfig, (kiss_fft_cpx*)&pFFTChunk->m_vvcfFFTChunks[uChannelIndex][0], (kiss_fft_cpx*)&pFFTChunk->m_vvcfFFTChunks[uChannelIndex][0]);
    }
    
    // Now free up memory
    kiss_fft_free(ForwardFFTConfig);

	TryPassChunk(pFFTChunk);
}