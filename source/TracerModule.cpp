#include "TracerModule.h"
#include <ChunkToJSONConverter.h>

TracerModule::TracerModule(nlohmann::json_abi_v3_11_2::json jsonConfig)
    : BaseModule(1000),
      m_strPipelinePositionName(
          CheckAndThrowJSON<std::string>(jsonConfig, "PipelinePostitionName")),
      m_bPrintJSONIfPossible(
          CheckAndThrowJSON<bool>(jsonConfig, "PrintJSONContent")) {


    RegisterChunkCallbackFunction(ChunkType::JSONChunk, &TracerModule::Process_Chunk, (BaseModule*)this);
    RegisterChunkCallbackFunction(ChunkType::TimeChunk, &TracerModule::Process_Chunk, (BaseModule*)this);
    RegisterChunkCallbackFunction(ChunkType::GPSChunk, &TracerModule::Process_Chunk, (BaseModule*)this);
    RegisterChunkCallbackFunction(ChunkType::JSONChunk, &TracerModule::Process_Chunk, (BaseModule*)this);

}

void TracerModule::Process_Chunk(std::shared_ptr<BaseChunk> pBaseChunk) {
  PLOG_DEBUG << m_strPipelinePositionName + ": " +
                    ChunkTypesNamingUtility::toString(
                        pBaseChunk->GetChunkType());

  if (m_bPrintJSONIfPossible) {
    if (ChunkToJSONConverter *pChunkToJSONConverter =
            dynamic_cast<ChunkToJSONConverter *>(pBaseChunk.get())) {
      auto pJSONChunk = std::make_shared<JSONChunk>();
      PLOG_DEBUG << pChunkToJSONConverter->ToJSON()->dump();
    } else if (pBaseChunk->GetChunkType() == ChunkType::JSONChunk) {
      auto pJSONChunk = std::static_pointer_cast<JSONChunk>(pBaseChunk);
      PLOG_DEBUG << pJSONChunk->m_JSONDocument.dump();
    }
  }

  TryPassChunk(pBaseChunk);
}
