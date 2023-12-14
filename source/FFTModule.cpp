#include "FFTModule.h"

FFTModule::FFTModule(unsigned uBufferSize) : BaseModule(uBufferSize)
{

}

void FFTModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    auto pTimeChunk = std::static_pointer_cast<TimeChunk>(pBaseChunk);
    auto pFFTChunk = std::make_shared<FFTChunk>(pTimeChunk->m_dChunkSize, pTimeChunk->m_dSampleRate, pTimeChunk->m_i64TimeStamp, pTimeChunk->m_uNumChannels);
    pFFTChunk->SetSourceIdentifier(pTimeChunk->GetSourceIdentifier());

    // Forward FFT configuration
    kiss_fft_cfg ForwardFFTConfig = kiss_fft_alloc(pTimeChunk->m_dChunkSize, 1, NULL, NULL);

    // Convert to complex vector
    pFFTChunk->m_vvcfFFTChunks.resize(pTimeChunk->m_uNumChannels);
    
    // Iterate through data channel
    for (uint16_t uChannelIndex = 0; uChannelIndex  < pTimeChunk->m_uNumChannels; uChannelIndex++)
    {

        std::vector<std::complex<float>> vcfForwardFFTInput;
        std::vector<std::complex<float>> vcfForwardFFTOutput(pTimeChunk->m_dChunkSize, 0.0);

        // And stoire time data in FFTChunk 
        for (auto i16SubChunkSample : pTimeChunk->m_vvi16TimeChunks[uChannelIndex])
            // Creating a complex value with only real component
            vcfForwardFFTInput.push_back(std::complex<float>(i16SubChunkSample, 0.0));

        // And then take FFT
        kiss_fft(ForwardFFTConfig, (kiss_fft_cpx*)&vcfForwardFFTInput[0], (kiss_fft_cpx*)&vcfForwardFFTOutput[0]);

        // And store data
        pFFTChunk->m_vvcfFFTChunks[uChannelIndex] = vcfForwardFFTOutput;
    }
    
    // Now free up memory
    kiss_fft_free(ForwardFFTConfig);

	TryPassChunk(pFFTChunk);
}