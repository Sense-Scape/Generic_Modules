#include "EnergyDetectionModule.h"

EnergyDetectionModule::EnergyDetectionModule(unsigned uBufferSize, float fThresholdAboveNoiseFoor) :m_fThresholdAboveNoiseFoor_db(fThresholdAboveNoiseFoor),
                                                                                                    BaseModule(uBufferSize)
{

}

void EnergyDetectionModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Check if it a chunk we are interested in otherwise try pass
    if(pBaseChunk->GetChunkType() != ChunkType::FFTMagnitudeChunk)
    {
        TryPassChunk(pBaseChunk);
        return;
    }
    auto pFFTMagnitudeChunk = std::static_pointer_cast<FFTMagnitudeChunk>(pBaseChunk);

    // Calculate power (square of magnitude) in DBx?
    auto pvvdPower_db = std::make_shared<std::vector<std::vector<double>>>(pFFTMagnitudeChunk->m_uNumChannels);
    for (unsigned uCurrentChannelIndex = 0; uCurrentChannelIndex < pFFTMagnitudeChunk->m_uNumChannels; uCurrentChannelIndex++)
    {
        for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < pFFTMagnitudeChunk->m_dChunkSize; uCurrentSampleIndex++)
        {
            auto mag = pFFTMagnitudeChunk->m_vvfFFTMagnitudeChunks[uCurrentChannelIndex][uCurrentSampleIndex];
            (*pvvdPower_db)[uCurrentChannelIndex].emplace_back(std::log(std::pow(mag, 2.0))/std::log(10));
        }
    }

    // Then calculate average power in dBx
    auto pvdAveragePower_db = std::make_shared<std::vector<double>>(pFFTMagnitudeChunk->m_uNumChannels);
    for (unsigned uCurrentChannelIndex = 0; uCurrentChannelIndex < pFFTMagnitudeChunk->m_uNumChannels; uCurrentChannelIndex++)
    {
        // divide becomes minus in log
        auto dAveragePower_db = std::log(std::accumulate((*pvvdPower_db)[uCurrentChannelIndex].begin(), (*pvvdPower_db)[uCurrentChannelIndex].end(), 0.0))/std::log(10) - std::log((*pvvdPower_db)[uCurrentChannelIndex].size())/std::log(10);
        (*pvdAveragePower_db)[uCurrentChannelIndex] = dAveragePower_db;
    }

    // Calculate detection threshold in dBx
    auto pvvDetectionThreshold_db = std::make_shared<std::vector<double>>(pFFTMagnitudeChunk->m_uNumChannels);
    for (unsigned uCurrentChannelIndex = 0; uCurrentChannelIndex < pFFTMagnitudeChunk->m_uNumChannels; uCurrentChannelIndex++)
    {
        (*pvvDetectionThreshold_db)[uCurrentChannelIndex] = (*pvdAveragePower_db)[uCurrentChannelIndex] +  m_fThresholdAboveNoiseFoor_db;
    }

    // try pass
    std::vector<std::vector<uint16_t>> vvu16DetectionBins;
    vvu16DetectionBins.resize(pFFTMagnitudeChunk->m_uNumChannels);

    // try pass
    // Iterate and store indicies of threshold
    for (unsigned uCurrentChannelIndex = 0; uCurrentChannelIndex < pFFTMagnitudeChunk->m_uNumChannels; uCurrentChannelIndex++)
    {

        for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < pFFTMagnitudeChunk->m_dChunkSize; uCurrentSampleIndex++)
        {

            auto mag = pFFTMagnitudeChunk->m_vvfFFTMagnitudeChunks[uCurrentChannelIndex][uCurrentSampleIndex];
            auto pow = std::log(std::pow(mag, 2.0))/std::log(10);

            // Now check and store
            if (pow > (*pvvDetectionThreshold_db)[uCurrentChannelIndex])
                vvu16DetectionBins[uCurrentChannelIndex].emplace_back(uCurrentSampleIndex);
            
        }
    }

    auto pDetecionChunk = std::make_shared<DetectionBinChunk>();
    pDetecionChunk->SetDetectionBins(vvu16DetectionBins);

    // try pass
    TryPassChunk(pDetecionChunk);
    TryPassChunk(pFFTMagnitudeChunk);
}