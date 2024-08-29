#include "TracerModule.h"

TracerModule::TracerModule(std::string strPipelinePosition) : BaseModule(1000),
                                                              m_strPipelinePosition(strPipelinePosition)
{
}

void TracerModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    PLOG_DEBUG << m_strPipelinePosition + ": " + ChunkTypesNamingUtility::toString(pBaseChunk->GetChunkType());

    if (ChunkToJSONConverter *pChunkToJSONConverter = dynamic_cast<ChunkToJSONConverter *>(pBaseChunk.get()))
	{
		auto pJSONChunk = std::make_shared<JSONChunk>();
		PLOG_DEBUG << pChunkToJSONConverter->ToJSON()->dump();
	}

    TryPassChunk(pBaseChunk);
}

void TracerModule::ContinuouslyTryProcess()
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