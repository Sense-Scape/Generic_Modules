#include "DirectionFindingModule.h"

DirectionFindingModule::DirectionFindingModule(unsigned uBufferSize, double dPropogationVelocity_mps, double dBaselineLength_m) : 
                                m_dPropogationVelocity_mps(dPropogationVelocity_mps),
                                m_dBaselineLength_m(dBaselineLength_m),
                                BaseModule(uBufferSize)
{

}

void DirectionFindingModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    switch (pBaseChunk->GetChunkType())
    {
    case ChunkType::DetectionBinChunk:
        ProcessDetectionBinChunk(pBaseChunk);
        break;
    
    case ChunkType::FFTChunk:
        ProcessFFTChunk(pBaseChunk);
        break;
    }

    TryPassChunk(pBaseChunk);
}

double DirectionFindingModule::AngleOfArrivalToDifferentialPhase(double f_hz, double differentialPhase_rads) {
    
    double lambda_m = m_dPropogationVelocity_mps / f_hz;
    double IntermediateCoefficient = 2 * M_PI * m_dBaselineLength_m / lambda_m;

    // Differential phase before scaling and wrapping
    double differentialPhase_rad = std::sin(differentialPhase_rad) * IntermediateCoefficient;
    // Scaled and wrapped differential phase
    differentialPhase_rad = std::fmod(differentialPhase_rad + M_PI, 2 * M_PI) - M_PI;

    return differentialPhase_rad;
}

void DirectionFindingModule::ProcessDetectionBinChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Store data for a single client
    auto pDetectionBinChunk = std::static_pointer_cast<DetectionBinChunk>(pBaseChunk);
    m_vvu16DetectionBins = *pDetectionBinChunk->GetDetectionBins();
}

void DirectionFindingModule::ProcessFFTChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    auto pFFTChunk = std::static_pointer_cast<FFTChunk>(pBaseChunk);

    // Use active bins to index FFT chunk
    for (unsigned uCurrentChannelIndex = 0; uCurrentChannelIndex < m_vvu16DetectionBins.size()-1; uCurrentChannelIndex++)
    {
        auto vu16DetectionBins0 = m_vvu16DetectionBins[uCurrentChannelIndex];
        auto vu16DetectionBins1 = m_vvu16DetectionBins[uCurrentChannelIndex+1];

        for (const auto& uDetectionIndex : vu16DetectionBins0)
        {
            // calcaulte differential phase
            auto A0 = pFFTChunk->m_vvcfFFTChunks[uCurrentChannelIndex][uDetectionIndex];
            auto A1 = pFFTChunk->m_vvcfFFTChunks[uCurrentChannelIndex+1][uDetectionIndex];
            auto a = differential_phase(A0, A1);
            
            ///PLOG_ERROR << std::to_string(a);

            // calculate AOA

            //  Print
        }
    }
}