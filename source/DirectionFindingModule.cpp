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

double DirectionFindingModule::CalculateAngleOfArrival(double differentialPhase_rads,double f_hz, double v_mps, double l_m) {
    
    double lambda_m = v_mps / f_hz;

    // Differential phase before scaling and wrapping
    double dAOA_rad = std::asin((lambda_m*differentialPhase_rads/(2*M_PI))/l_m);
    
    return dAOA_rad;
}

void DirectionFindingModule::ProcessDetectionBinChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Store data for a single client
    auto pDetectionBinChunk = std::static_pointer_cast<DetectionBinChunk>(pBaseChunk);
    m_vvu16DetectionBins = *pDetectionBinChunk->GetDetectionBins();
}

float DirectionFindingModule::CalculateDifferentialPhase(const std::complex<float>& z1, const std::complex<float>& z2)
{
        // Ensure non-zero magnitude to avoid division by zero
        if (std::abs(z2) < std::numeric_limits<float>::epsilon()) {
            throw std::invalid_argument("Second complex number (z2) cannot have zero magnitude");
        }

        // Calculate the phase difference
        float phase_diff = std::atan2(std::imag(z1), std::real(z1)) -
                            std::atan2(std::imag(z2), std::real(z2));

        // Wrap the phase difference to the range (-pi, pi]
        phase_diff = std::fmod(phase_diff + M_PI, 2.0f * M_PI) - M_PI;

        return phase_diff;
}

void DirectionFindingModule::ProcessFFTChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    auto pFFTChunk = std::static_pointer_cast<FFTChunk>(pBaseChunk);
    
    // First we start with checks in debug mode
    assert(pFFTChunk->m_vvcfFFTChunks.size() != 0);
    assert(pFFTChunk->m_dSampleRate != 0);

    if (pFFTChunk->m_vvcfFFTChunks.size() <= 1)
    {
        TryPassChunk(pFFTChunk);
        return;
    }
    
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

        auto fDeltaA_rad = CalculateDifferentialPhase(cfA0, cfA1);
        auto dFrequencyOfInterest_hz = uDetectionIndex*pFFTChunk->m_dSampleRate/(2.0f*(pFFTChunk->m_dChunkSize-1));

        auto dAOA_rad = CalculateAngleOfArrival(fDeltaA_rad, dFrequencyOfInterest_hz, m_dPropogationVelocity_mps, m_dBaselineLength_m);
        auto dAOA_deg = dAOA_rad*180/M_PI;

        vfAngleOfArrivals.emplace_back(dAOA_deg);
    }

    // And store said data
    auto pDirectionChunk = std::make_shared<DirectionBinChunk>();
    pDirectionChunk->SetSourceIdentifier(pFFTChunk->GetSourceIdentifier());
    pDirectionChunk->SetDirectionData(m_vvu16DetectionBins[0].size(), m_vvu16DetectionBins[0], vfAngleOfArrivals, pFFTChunk->m_dSampleRate);

    // Once complete, pass on the data
    TryPassChunk(pFFTChunk);
    TryPassChunk(pDirectionChunk);
}