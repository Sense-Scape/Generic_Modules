#include "ToJSONModule.h"

ToJSONModule::ToJSONModule()
{

}

void ToJSONModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	// Check if chunk type IS A ChunkToJSONConverter, otherwise drop processing
	if (ChunkToJSONConverter* pChunkToJSONConverter = dynamic_cast<ChunkToJSONConverter*>(pBaseChunk.get()))
	{
		auto pJSONChunk = std::make_shared<JSONChunk>();
		pJSONChunk->m_JSONDocument = *pChunkToJSONConverter->ToJSON();

		// Try pass on
		TryPassChunk(pJSONChunk);
	}
}