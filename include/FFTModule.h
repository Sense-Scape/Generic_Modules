#ifndef FFT_MODULE
#define FFT_MODULE

/*Standard Includes*/

/* Custom Includes */
#include "BaseModule.h"
#include "TimeChunk.h"
#include "FFTChunk.h"
#include "FFTMagnitudeChunk.h"
#include "kiss_fftr.h"

/**
 * @brief Converts time chunks to FFT chunks and computes FFT
 */
class FFTModule : public BaseModule
{
public:
    /**
     * @brief Construct a new FFTModule object
     * @param uBufferSize size of processing input buffer
     */
    FFTModule(unsigned uBufferSize);

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    std::string GetModuleType() override { return "FFTModule"; };

    /**
     * @brief Determines whether the module will genreate FFT magnitude data
     */
    void SetGenerateMagnitudeData(bool bGenerateMagnitudeData) { m_bGenerateMagnitudeData = bGenerateMagnitudeData; }

private:
    std::atomic<bool> m_bGenerateMagnitudeData = false; ///< Whether fft magnitude chunks will be produced
};

#endif