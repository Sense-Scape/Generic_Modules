#ifndef TO_JSON_MODULE
#define TO_JSON_MODULE

#include "BaseModule.h"
#include "JSONChunk.h"
#include "ChunkToJSONConverter.h"

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
    std::string GetModuleType() override { return "ToJSONModule"; };

private:
};

#endif