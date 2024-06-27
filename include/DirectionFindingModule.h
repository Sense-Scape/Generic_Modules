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

    /**
     * @brief Calculates the expected differential phase for a given angle of arrival (AOA).
     *
     * This function calculates the scaled differential phase in radians for a single
     * angle of arrival (AOA) based on the baseline length (L_m), velocity of propagation
     * (v_mps), and frequency of interest (f_hz).
     *
     * @param f_hz [in] Frequency of interest in hertz.
     * @param differentialPhase_rad [in] Angle of arrival in radians.
     *
     * @return The scaled differential phase in radians.
     */
    double AngleOfArrivalToDifferentialPhase(double f_hz, double differentialPhase_rad);

    float CalculateDifferentialPhase(const std::complex<float>& z1, const std::complex<float>& z2) {
        // Ensure non-zero magnitude to avoid division by zero
        if (std::abs(z2) < std::numeric_limits<float>::epsilon()) {
            throw std::invalid_argument("Second complex number (z2) cannot have zero magnitude");
        }

        // Calculate the phase difference
        float phase_diff = std::atan2(std::imag(z1), std::real(z1)) -
                            std::atan2(std::imag(z2), std::real(z2));

        // Wrap the phase difference to the range (-pi, pi]
        phase_diff = std::fmod(phase_diff + M_PI, 2.0f * M_PI) - M_PI;

        return phase_diff*180/M_PI;
    }

    void ProcessFFTChunk(std::shared_ptr<BaseChunk> pBaseChunk);
    void ProcessDetectionBinChunk(std::shared_ptr<BaseChunk> pBaseChunk);

};

#endif