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
 * @brief Converts time chunks to FFT chunks and computes FFT
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
   
};

#endif