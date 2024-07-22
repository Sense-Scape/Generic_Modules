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
 * @brief Calculates direction information using detection indicies and FFTs
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

    double m_dPropogationVelocity_mps;  ///< Propogation velocity of medium
    double m_dBaselineLength_m;         ///< Baseline length between 2 recievers
    std::map<std::vector<uint8_t>,std::vector<std::vector<uint16_t>>> m_mSourceIdentierToDetectionBins; ///< Previous detections for particular source

    /**
     * @brief Given two complex frequencies, differential phase will be returned
     * @param z1 Complex magnitude of first channel
     * @param z2 Complex magnitude of second channel
     * @return differential phase in radians
     */
    float CalculateDifferentialPhase(const std::complex<float>& z1, const std::complex<float>& z2);

    /**
     * @brief Calcualtes direction of arrival in radians
     * @param differentialPhase_rads differential phase in radians
     * @param f_hz freqeuncy of phase in Herz
     * @param v_mps velocity of propogation in medium in meters per second
     * @param l_m baseline length in meters
     * @return The angle of arrival in radians
     */
    double CalculateAngleOfArrival(double differentialPhase_rads,double f_hz, double v_mps, double l_m);

    /**
     * @brief Will calculate AOA using detection indicies and complex FFT magnitudes
     * @param pBaseChunk pointer to FFT chunk
     */
    void ProcessFFTChunk(std::shared_ptr<BaseChunk> pBaseChunk);

    /**
     * @brief Will update detection indicies of current source identifier
     * @param pBaseChunk pointer to Energy Detection chunk
     */
    void ProcessDetectionBinChunk(std::shared_ptr<BaseChunk> pBaseChunk);

};

#endif