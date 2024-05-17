#include "ToJSONModule.h"

ToJSONModule::ToJSONModule()
{
}

void ToJSONModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	std::cout << "ff" << std::endl;
	// Check if chunk type IS A ChunkToJSONConverter, otherwise drop processing
	if (ChunkToJSONConverter *pChunkToJSONConverter = dynamic_cast<ChunkToJSONConverter *>(pBaseChunk.get()))
	{
		auto pJSONChunk = std::make_shared<JSONChunk>();
		pJSONChunk->m_JSONDocument = *pChunkToJSONConverter->ToJSON();

		std::cout << pJSONChunk->m_JSONDocument.dump() << std::endl;
		// Try pass on
		TryPassChunk(pJSONChunk);
	}
	else
	{
		auto strChunkType = ChunkTypesNamingUtility::toString(pBaseChunk->GetChunkType());
		std::string strWarning = std::string(__FUNCTION__) + ": " + strChunkType + " has no JSON conversionn, droppong chunk \n";
		PLOG_WARNING << strWarning;
	}
}