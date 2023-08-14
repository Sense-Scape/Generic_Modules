#ifndef TO_JSON_MODULE
#define TO_JSON_MODULE

#include "BaseModule.h"

#include "TimeChunk.h"
#include "JSONChunk.h"

/**
 * @brief Converts just to the JSON Chunk type
 */
class ToJSONModule : public BaseModule
{
public:
    /**
     * @brief Construct a new ToJSONModule object
     */
    ToJSONModule();
    //~ToJSONModule(){};

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::HPFModule; };

private:
    

};

#endif