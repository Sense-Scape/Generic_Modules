#ifndef TRACER_MODULE
#define TRACER_MODULE

#include "BaseModule.h"
#include "JSONChunk.h"
#include "ChunkToJSONConverter.h"

/**
 * @brief Converts just to the JSON Chunk type
 */
class TracerModule : public BaseModule
{
public:
    /**
     * @brief Construct a new TracerModule object
     */
    TracerModule(std::string strPipelinePosition);
    //~TracerModule(){};

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    std::string GetModuleType() override { return "TracerModule"; };

private:
    std::string m_strPipelinePosition; ///< Name printed with tracer
};

#endif