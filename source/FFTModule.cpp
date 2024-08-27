#include "FFTModule.h"

FFTModule::FFTModule(unsigned uBufferSize) : BaseModule(uBufferSize)
{
    RegisterChunkCallbackFunction(ChunkType::TimeChunk, &FFTModule::Process_TimeChunk); 
}

void FFTModule::Process_TimeChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    auto pTimeChunk = std::static_pointer_cast<TimeChunk>(pBaseChunk);
    auto pFFTChunk = std::make_shared<FFTChunk>(pTimeChunk->m_dChunkSize/2 + 1, pTimeChunk->m_dSampleRate, pTimeChunk->m_i64TimeStamp, pTimeChunk->m_uNumChannels);
    pFFTChunk->SetSourceIdentifier(pTimeChunk->GetSourceIdentifier());

    // Forward FFT configuration
    kiss_fftr_cfg ForwardFFTConfig = kiss_fftr_alloc(pTimeChunk->m_dChunkSize, 0, NULL, NULL);

    // Convert to complex vector
    pFFTChunk->m_vvcfFFTChunks.resize(pTimeChunk->m_uNumChannels);

    // Iterate through data channel
    for (uint16_t uChannelIndex = 0; uChannelIndex < pTimeChunk->m_uNumChannels; uChannelIndex++)
    {

        std::vector<float> vcfForwardFFTInput;
        std::vector<std::complex<float>> vcfForwardFFTOutput(pTimeChunk->m_dChunkSize/2 +1, 0.0);

        // And stoire time data in FFTChunk 
        for (auto i16SubChunkSample : pTimeChunk->m_vvi16TimeChunks[uChannelIndex])
            // Creating a complex value with only real component
            vcfForwardFFTInput.emplace_back(i16SubChunkSample);

        // And then take FFT
        kiss_fftr(ForwardFFTConfig, vcfForwardFFTInput.data(), (kiss_fft_cpx*)&vcfForwardFFTOutput[0]);

        // Rescale according to FFT gain
        for (size_t i = 0; i < vcfForwardFFTOutput.size(); ++i)
            vcfForwardFFTOutput[i] /= vcfForwardFFTOutput.size(); // Scale each element by 5

        // And store data
        pFFTChunk->m_vvcfFFTChunks[uChannelIndex] = vcfForwardFFTOutput;
    }

    // Now free up memory
    free(ForwardFFTConfig);


    if (m_bGenerateMagnitudeData) {

        // Create the chunk with attention to soruce identifier
        auto pFFTMagnitudeChunk = std::make_shared<FFTMagnitudeChunk>(pTimeChunk->m_dChunkSize/2 + 1, pTimeChunk->m_dSampleRate, pTimeChunk->m_i64TimeStamp, pTimeChunk->m_uNumChannels);
        pFFTMagnitudeChunk->SetSourceIdentifier(pTimeChunk->GetSourceIdentifier());

        // Compute FFT magnitudes
        for (uint16_t uChannelIndex = 0; uChannelIndex < pTimeChunk->m_uNumChannels; ++uChannelIndex) {
            std::transform(pFFTChunk->m_vvcfFFTChunks[uChannelIndex].begin(), pFFTChunk->m_vvcfFFTChunks[uChannelIndex].end(), pFFTMagnitudeChunk->m_vvfFFTMagnitudeChunks[uChannelIndex].begin(),
                [](std::complex<float> x) { return std::abs(x); });
        }

        TryPassChunk(pFFTMagnitudeChunk);
    }
    
    TryPassChunk(pTimeChunk);
    TryPassChunk(pFFTChunk);
}