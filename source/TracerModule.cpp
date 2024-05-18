#include "TracerModule.h"

TracerModule::TracerModule()
{
}

void TracerModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    PLOG_DEBUG << ChunkTypesNamingUtility::toString(pBaseChunk->GetChunkType());
    TryPassChunk(pBaseChunk);
}