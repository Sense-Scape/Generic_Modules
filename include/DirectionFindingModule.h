#ifndef DIRECTION_FINDING_MODULE
#define DIRECTION_FINDING_MODULE

/*Standard Includes*/
#include <cmath>

/* Custom Includes */
#include "BaseModule.h"
#include "DetectionBinChunk.h"
#include "DirectionBinChunk.h"
#include "FFTChunk.h"
#include "kiss_fft.h"


/**
 * @brief Converts time chunks to FFT chunks and computes FFT
 */
class DirectionFindingModule : public BaseModule
{
public:
    /**
     * @brief Construct a new DirectionFindingModule object
     * @param uBufferSize size of processing input buffer
     */
    DirectionFindingModule(unsigned uBufferSize, double dPropogationVelocity_mps, double dBaselineLength_m);

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    std::string GetModuleType() override { return "DirectionFindingModule"; };

private:

    double m_dPropogationVelocity_mps;
    double m_dBaselineLength_m;
    std::vector<std::vector<uint16_t>> m_vvu16DetectionBins;

    double CalculateAngleOfArrival(double differentialPhase_rads,double f_hz, double v_mps, double l_m);

    float CalculateDifferentialPhase(const std::complex<float>& z1, const std::complex<float>& z2);

    void ProcessFFTChunk(std::shared_ptr<BaseChunk> pBaseChunk);
    void ProcessDetectionBinChunk(std::shared_ptr<BaseChunk> pBaseChunk);

};

#endif