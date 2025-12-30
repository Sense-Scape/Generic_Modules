#ifndef TRACER_MODULE
#define TRACER_MODULE

#include "BaseModule.h"

/**
 * @brief Prints out all chunks or chunk data passing through
 */
class TracerModule : public BaseModule {
public:
  /**
   * @brief Construct a new TracerModule object
   */
  TracerModule(nlohmann::json_abi_v3_11_2::json jsonConfig);
  //~TracerModule(){};

  /**
   * @brief Returns module type
   * @param[out] ModuleType of processing module
   */
  std::string GetModuleType() override { return "TracerModule"; };

private:
  const std::string m_strPipelinePositionName; ///< Name printed with tracer
  const bool m_bPrintJSONIfPossible; ///< If json availablle, will be printed

  /**
   * @brief Generate and fill complex time data chunk and pass on to next module
   */
  void Process_Chunk(std::shared_ptr<BaseChunk> pBaseChunk);
};

#endif
