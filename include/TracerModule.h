#ifndef TRACER_MODULE
#define TRACER_MODULE

#include "BaseModule.h"
#include "JSONChunk.h"
#include "ChunkToJSONConverter.h"

/**
 * @brief Prints out all chunks or chunk data passing through
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
     * @brief Check input buffer and try process data
     */
    void ContinuouslyTryProcess() override;

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    std::string GetModuleType() override { return "TracerModule"; };

    /**
     * @brief Sets Whether to print JSON data or not
     * @param[out] bPrintJSONIfPossible Whether to print JSON data or not
     */
    void SetPrintJSON(bool bPrintJSONIfPossible) { m_bPrintJSONIfPossible = bPrintJSONIfPossible; };

private:
    std::string m_strPipelinePosition;  ///< Name printed with tracer
    bool m_bPrintJSONIfPossible;        ///< If json availablle, will be printed

        /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk);
};

#endif