#include "TracerModule.h"

TracerModule::TracerModule(std::string strPipelinePosition) : BaseModule(1000),
                                                              m_strPipelinePosition(strPipelinePosition)
{
}

void TracerModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    PLOG_DEBUG << m_strPipelinePosition + ": " + ChunkTypesNamingUtility::toString(pBaseChunk->GetChunkType());
    TryPassChunk(pBaseChunk);
}