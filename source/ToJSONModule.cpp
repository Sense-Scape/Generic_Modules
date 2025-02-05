#include "ToJSONModule.h"

ToJSONModule::ToJSONModule(unsigned uBufferSize) : BaseModule(uBufferSize) 
{
}

void ToJSONModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	// Check if chunk type IS A ChunkToJSONConverter, otherwise drop processing
	if (ChunkToJSONConverter *pChunkToJSONConverter = dynamic_cast<ChunkToJSONConverter *>(pBaseChunk.get()))
	{
		auto pJSONChunk = std::make_shared<JSONChunk>();
		pJSONChunk->m_JSONDocument = *pChunkToJSONConverter->ToJSON();
		// Try pass on
		TryPassChunk(pJSONChunk);
		//std::cout << pJSONChunk->m_JSONDocument.dump() << std::endl;
	}
	else if(pBaseChunk->GetChunkType() == ChunkType::JSONChunk)
	{
		TryPassChunk(pBaseChunk);
	}
	else
	{
		auto strChunkType = ChunkTypesNamingUtility::toString(pBaseChunk->GetChunkType());
		std::string strWarning = std::string(__FUNCTION__) + ": " + strChunkType + " has no JSON conversionn, droppong chunk \n";
		PLOG_WARNING << strWarning;
	}
}

void ToJSONModule::ContinuouslyTryProcess()
{
    while (!m_bShutDown)
    {
        std::shared_ptr<BaseChunk> pBaseChunk;
        if (TakeFromBuffer(pBaseChunk))
            Process(pBaseChunk);
        else
        {
            // Wait to be notified that there is data available
            std::unique_lock<std::mutex> BufferAccessLock(m_BufferStateMutex);
            m_cvDataInBuffer.wait_for(BufferAccessLock, std::chrono::milliseconds(1),  [this] {return (!m_cbBaseChunkBuffer.empty() || m_bShutDown);});
        }
    }
}