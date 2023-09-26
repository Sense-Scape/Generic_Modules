#ifndef FFT_MODULE
#define FFT_MODULE

/*Standard Includes*/

/* Custom Includes */
#include "BaseModule.h"
#include "TimeChunk.h"
#include "FFTChunk.h"

/**
 * @brief Converts just to the JSON Chunk type
 */
class FFTModule : public BaseModule
{
public:
    /**
     * @brief Construct a new FFTModule object
     */
    FFTModule();

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::FFTModule; };

private:


};

#endif