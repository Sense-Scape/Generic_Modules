#include "ToJSONModule.h"

ToJSONModule::ToJSONModule()
{

}

void ToJSONModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	// Assuming we are just processing time chunks at the moment
	auto pTimeChunk = std::static_pointer_cast<TimeChunk>(pBaseChunk);
	auto pJSONChunk = std::make_shared<JSONChunk>();

	auto JSONDocument = nlohmann::json();
	auto strChunkName = ChunkTypesNamingUtility::toString(pTimeChunk->GetChunkType());
	JSONDocument[strChunkName]["SampleRate"] = std::to_string(pTimeChunk->m_dSampleRate);
	JSONDocument[strChunkName]["ChunkSize"] = std::to_string(pTimeChunk->m_dChunkSize);
	JSONDocument[strChunkName]["NumChannels"] = std::to_string(pTimeChunk->m_uNumChannels);

	for (unsigned uChannelIndex = 0; uChannelIndex < pTimeChunk->m_uNumChannels; uChannelIndex++)
		JSONDocument[strChunkName]["Channels"][std::to_string(uChannelIndex)] = pTimeChunk->m_vvi16TimeChunks[uChannelIndex];

	pJSONChunk->m_JSONDocument = JSONDocument;

	// Try pass on
	TryPassChunk(pJSONChunk);
}