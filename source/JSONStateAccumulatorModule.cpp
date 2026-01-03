
#include "JSONStateAccumulatorModule.h"

JSONStateAccumulatorModule::JSONStateAccumulatorModule(
    unsigned uBufferSize, nlohmann::json_abi_v3_11_2::json jsonConfig)
    : BaseModule(uBufferSize),
      m_bEnableJSONLogs(CheckAndThrowJSON<bool>(jsonConfig, "EnableJSONLogs")),
      m_dReportingPeriod_s(
          CheckAndThrowJSON<double>(jsonConfig, "ReportingPeriod_s")) {
  RegisterChunkCallbackFunction(ChunkType::JSONChunk,
                                &JSONStateAccumulatorModule::Process_JSONChunk,
                                (BaseModule *)this);
}

void JSONStateAccumulatorModule::StartProcessing() {
  if (!m_thread.joinable()) {
    m_thread = std::thread([this]() { ContinuouslyTryProcess(); });

    m_QueueJSONTxThread = std::thread([this]() { WaitAndSendJSONDocuments(); });

    std::string strInfo = std::string(__FUNCTION__) + " " + GetModuleType() +
                          ": Processing thread started";
    PLOG_WARNING << strInfo;
  } else {
    // Log warning
    std::string strWarning =
        std::string(__FUNCTION__) + ": Processing thread already started";
    PLOG_WARNING << strWarning;
  }
}

void JSONStateAccumulatorModule::WaitAndSendJSONDocuments() {
  while (!m_bShutDown) {
    // Iterate over all stored JSON states by source identifier
    for (const auto &pair : m_JSONStatesBySource) {
      const std::vector<uint8_t> &vu8SourceID = pair.first;
      const auto &jsonCurrentState = pair.second;

      auto pJSONChunkofCurrentState = std::make_shared<JSONChunk>();
      pJSONChunkofCurrentState->SetSourceIdentifier(vu8SourceID);
      pJSONChunkofCurrentState->m_JSONDocument = jsonCurrentState;

      if (m_bEnableJSONLogs)
        PLOG_DEBUG << jsonCurrentState.dump();

      TryPassChunk(pJSONChunkofCurrentState);
    }
    std::this_thread::sleep_for(
        std::chrono::milliseconds(uint64_t(m_dReportingPeriod_s * 1000)));
  }
}

void JSONStateAccumulatorModule::Process_JSONChunk(
    std::shared_ptr<BaseChunk> pBaseChunk) {
  auto pJSONChunk = std::dynamic_pointer_cast<JSONChunk>(pBaseChunk);
  auto vu8SourceID = pJSONChunk->GetSourceIdentifier();

  auto &jsonCurrentState = m_JSONStatesBySource[vu8SourceID];

  for (auto itNewJSONChunk = pJSONChunk->m_JSONDocument.begin();
       itNewJSONChunk != pJSONChunk->m_JSONDocument.end(); ++itNewJSONChunk) {
    auto key = itNewJSONChunk.key();
    auto value = itNewJSONChunk.value();
    jsonCurrentState[key] = value;
  }
}
