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
    
    // First we start with checks in debug mode
    assert(pFFTChunk->m_vvcfFFTChunks.size() != 0);
    assert(pFFTChunk->m_dSampleRate != 0);

    // Prep the data to process
    std::vector<float> vfAngleOfArrivals;
    auto vu16DetectionBins0 = m_vvu16DetectionBins[0];
    auto vu16DetectionBins1 = m_vvu16DetectionBins[1];

    // Then calculate the AOA
    for (const auto& uDetectionIndex : vu16DetectionBins0)
    {
        // calcaulte differential phase
        auto cfA0 = pFFTChunk->m_vvcfFFTChunks[0][uDetectionIndex];
        auto cfA1 = pFFTChunk->m_vvcfFFTChunks[1][uDetectionIndex];
        auto fDeltaA = CalculateDifferentialPhase(cfA0, cfA1);
            
        vfAngleOfArrivals.emplace_back(fDeltaA);
    }

    // And store said data
    auto pDirectionChunk = std::make_shared<DirectionBinChunk>();
    pDirectionChunk->SetSourceIdentifier(pFFTChunk->GetSourceIdentifier());
    pDirectionChunk->SetDirectionData(m_vvu16DetectionBins[0].size(), m_vvu16DetectionBins[0], vfAngleOfArrivals, pFFTChunk->m_dSampleRate);

    // Once complete, pass on the data
    TryPassChunk(pFFTChunk);
    TryPassChunk(pDirectionChunk);
}