#ifndef FFT_MODULE
#define FFT_MODULE

/*Standard Includes*/

/* Custom Includes */
#include "BaseModule.h"
#include "TimeChunk.h"
#include "FFTChunk.h"
#include "kiss_fft.h"

/**
 * @brief Converts just to the JSON Chunk type
 */
class FFTModule : 
    public BaseModule
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
    ModuleType GetModuleType() override { return ModuleType::FFTModule; };
};

#endif