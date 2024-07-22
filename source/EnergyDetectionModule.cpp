#include "EnergyDetectionModule.h"

EnergyDetectionModule::EnergyDetectionModule(unsigned uBufferSize, float fThresholdAboveNoiseFoor) :m_fThresholdAboveNoiseFoor_db(fThresholdAboveNoiseFoor),
                                                                                                    BaseModule(uBufferSize)
{

}

void EnergyDetectionModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Check if it a chunk we are interested in otherwise try pass
    if(pBaseChunk->GetChunkType() != ChunkType::FFTMagnitudeChunk) {
        TryPassChunk(pBaseChunk);
        return;
    }

    // Now we prepare the chunk for processing
    auto pFFTMagnitudeChunk = std::static_pointer_cast<FFTMagnitudeChunk>(pBaseChunk);

    // Run detection algorithim
    auto pvvdPower_db = CalculateBinPowerInDBX(pFFTMagnitudeChunk);
    auto pvdAveragePower_db = CalculateAverageBinPowerInDBX(pvvdPower_db);
    auto dDetectionThreshold = CalculateThresholdInDBX(pvdAveragePower_db->at(0), m_fThresholdAboveNoiseFoor_db);
    auto pvvu16DetectionBins = GetDetectionBinIndicies(pFFTMagnitudeChunk, dDetectionThreshold);

    // Generate chunk
    auto pDetecionChunk = std::make_shared<DetectionBinChunk>();
    pDetecionChunk->SetSourceIdentifier(pFFTMagnitudeChunk->GetSourceIdentifier());
    pDetecionChunk->SetDetectionBins((*pvvu16DetectionBins) );

    // try pass
    TryPassChunk(pDetecionChunk);
    TryPassChunk(pFFTMagnitudeChunk);
}

std::shared_ptr<std::vector<std::vector<double>>> EnergyDetectionModule::CalculateBinPowerInDBX(std::shared_ptr<FFTMagnitudeChunk> pFFTMagnitudeChunk)
{
    auto pvvdPower_db = std::make_shared<std::vector<std::vector<double>>>(pFFTMagnitudeChunk->m_uNumChannels);
    for (unsigned uCurrentChannelIndex = 0; uCurrentChannelIndex < pFFTMagnitudeChunk->m_uNumChannels; uCurrentChannelIndex++)
    {
        for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < pFFTMagnitudeChunk->m_dChunkSize; uCurrentSampleIndex++)
        {
            auto mag = pFFTMagnitudeChunk->m_vvfFFTMagnitudeChunks[uCurrentChannelIndex][uCurrentSampleIndex];
            (*pvvdPower_db)[uCurrentChannelIndex].emplace_back(std::log(std::pow(mag, 2.0))/std::log(10));
        }
    }

    return pvvdPower_db;
}

std::shared_ptr<std::vector<double>> EnergyDetectionModule::CalculateAverageBinPowerInDBX(std::shared_ptr<std::vector<std::vector<double>>> pvvdPower_db)
{
    auto pvdAveragePower_db = std::make_shared<std::vector<double>>(pvvdPower_db->size());
    for (unsigned uCurrentChannelIndex = 0; uCurrentChannelIndex < pvvdPower_db->size(); uCurrentChannelIndex++)
    {
        // divide becomes minus in log
        auto dAveragePower_db = std::log(std::accumulate((*pvvdPower_db)[uCurrentChannelIndex].begin(), (*pvvdPower_db)[uCurrentChannelIndex].end(), 0.0))/std::log(10) - std::log((*pvvdPower_db)[uCurrentChannelIndex].size())/std::log(10);
        (*pvdAveragePower_db)[uCurrentChannelIndex] = dAveragePower_db;
    }

    return pvdAveragePower_db;
}

double EnergyDetectionModule::CalculateThresholdInDBX(double dAveragePower_db, double dLevelAboveFloor)
{
    // Wrapper for simple calculation at this point. more complex implementations coming in future
    return dAveragePower_db + dLevelAboveFloor;
}

std::shared_ptr<std::vector<std::vector<uint16_t>>> EnergyDetectionModule::GetDetectionBinIndicies(std::shared_ptr<FFTMagnitudeChunk> pFFTMagnitudeChunk, double dDetectionThreshold)
{
    auto pvvu16DetectionBins = std::make_shared<std::vector<std::vector<uint16_t>>>();
    pvvu16DetectionBins->resize(pFFTMagnitudeChunk->m_uNumChannels);

    // try pass
    // Iterate and store indicies of threshold
    for (unsigned uCurrentChannelIndex = 0; uCurrentChannelIndex < pFFTMagnitudeChunk->m_uNumChannels; uCurrentChannelIndex++)
    {

        for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < pFFTMagnitudeChunk->m_dChunkSize; uCurrentSampleIndex++)
        {

            auto mag = pFFTMagnitudeChunk->m_vvfFFTMagnitudeChunks[uCurrentChannelIndex][uCurrentSampleIndex];
            auto pow = std::log(std::pow(mag, 2.0))/std::log(10);

            // Now check and store
            if (pow > dDetectionThreshold)
                (*pvvu16DetectionBins)[uCurrentChannelIndex].emplace_back(uCurrentSampleIndex);
            
        }
    }

    return pvvu16DetectionBins;
}