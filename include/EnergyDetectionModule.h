#ifndef ENERGY_DETECTION_MODULE
#define ENERGY_DETECTION_MODULE

/*Standard Includes*/
#include <cmath>

/* Custom Includes */
#include "BaseModule.h"
#include "DetectionBinChunk.h"
#include "FFTMagnitudeChunk.h"
#include "kiss_fft.h"


/**
 * @brief Calculates indicies where energy in detected
 */
class EnergyDetectionModule : public BaseModule
{
public:

    /**
     * @brief Construct a new EnergyDetectionModule object
     * @param uBufferSize size of processing input buffer
     * @param fThresholdAboveNoiseFoor Threshold above average power to detect signals
     */
    EnergyDetectionModule(unsigned uBufferSize, float fThresholdAboveNoiseFoor);

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    std::string GetModuleType() override { return "EnergyDetectionModule"; };

    /**
     * @brief threshold above average power to detect signals
     */
    void SetThresholdAboveNoiseFoor(float fThresholdAboveNoiseFoor) { m_fThresholdAboveNoiseFoor_db = fThresholdAboveNoiseFoor; }

private:
    std::atomic<float> m_fThresholdAboveNoiseFoor_db;  ///< Threshold above average power to detect signals

    /**
     * @brief Calculate power (dB) in each bin for FFT chunk
     * @param pFFTMagnitudeChunk Pointer to FFT chunk
     * @return pointer to vector of channelvectors and their bin powers
     */
    std::shared_ptr<std::vector<std::vector<double>>> CalculateBinPowerInDBX(std::shared_ptr<FFTMagnitudeChunk> pFFTMagnitudeChunk);

    /**
     * @brief Will give a average power in (dB) of each vector ccontained in the outter vector
     * @param pvvdPower_db Vector of vectors containing bin power
     * @return Vector containing average power of each vector passed in
     */
    std::shared_ptr<std::vector<double>> CalculateAverageBinPowerInDBX(std::shared_ptr<std::vector<std::vector<double>>> pvvdPower_db);

    /**
     * @brief Calculates the energy detection threshold (in dB)
     * @param dAveragePower_db average power (dB) of a vector of bins
     * @param dLevelAboveFloor Threshold above average (in dB) to detect energy
     * @return The detection threshold in (dB) to consider signal detected
     */
    double CalculateThresholdInDBX(double dAveragePower_db, double dLevelAboveFloor);

    /**
     * @brief Calculates indicies of where power detection occurs
     * @param pFFTMagnitudeChunk Pointer to FFT magnitude chunk
     * @param dDetectionThreshold The detection threshold in (dB) to consider signal detected
     * @return Vector of vectors containing detection indicies for given power vector
     */
    std::shared_ptr<std::vector<std::vector<uint16_t>>> GetDetectionBinIndicies(std::shared_ptr<FFTMagnitudeChunk> pFFTMagnitudeChunk,double dDetectionThreshold);
};

#endif