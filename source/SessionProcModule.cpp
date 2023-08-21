#include "SessionProcModule.h"

SessionProcModule::SessionProcModule(unsigned uBufferSize) : BaseModule(uBufferSize),
                                                            m_mFunctionCallbacksMap(),
                                                            m_mSessionBytes()
{
}

void SessionProcModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Lets first check what chunk has been transmitted in this UDP chunk
    uint32_t u32ChunkType;
    auto pUDPChunk = std::static_pointer_cast<UDPChunk>(pBaseChunk);
    memcpy(&u32ChunkType, &pUDPChunk->m_vcDataChunk[9], sizeof(u32ChunkType));

    // Then we can map keys
    ChunkType SessionChunkType = ChunkTypesNamingUtility::FromU32(u32ChunkType);
    auto vu8SourceIdentifier = pUDPChunk->GetSourceIdentifier();
    auto  pChunkHeaderState = GetPreviousSessionState(pUDPChunk, SessionChunkType);

    pChunkHeaderState->ConvertBytesToStates(pUDPChunk);

    // Now we can check all state variables
    // Are we the first message in a sequence ?
    bool bStartSequence = (pChunkHeaderState->m_puSequenceNumber.second == 0);
    // If we are we then want to know if that sequence in continuous
    bool bSequenceContinuous = (pChunkHeaderState->m_puSequenceNumber.second == pChunkHeaderState->m_uPreviousSequenceNumber + 1); //intra
    // Now if we know about continuity, is this the last message in the sequence?
    bool bLastInSequence = pChunkHeaderState->m_pcTransmissionState.second == 1 ? true : false;
    // Checking if this message belongs to the same sequences
    bool SameSesession = ((pChunkHeaderState->m_puSessionNumber.second == pChunkHeaderState->m_uPreviousSessionNumber) || pChunkHeaderState->m_puSessionNumber.second == 0); //inter

    if (bStartSequence)
        pChunkHeaderState->m_uPreviousSessionNumber = pChunkHeaderState->m_puSessionNumber.second;

    // Updating previous sequence after required for continuity/start checks
    pChunkHeaderState->m_uPreviousSequenceNumber = pChunkHeaderState->m_puSequenceNumber.second;

    // We have just started or are continuing a sequence so store intermediate bytes
    if (bStartSequence || (SameSesession && !bLastInSequence && bSequenceContinuous))
    {
        // If this is the start create a vector to store data
        if (bStartSequence)
            m_mSessionBytes[vu8SourceIdentifier][SessionChunkType] = std::make_shared<std::vector<char>>();

        // lets get the start and end of the data and store the bytes
        auto DataStart = pUDPChunk->m_vcDataChunk.begin() + pChunkHeaderState->m_uDataStartPosition;
        auto DataEnd = pUDPChunk->m_vcDataChunk.begin() + pChunkHeaderState->m_uDataStartPosition + pChunkHeaderState->m_puTransmissionSize.second;
        std::copy(DataStart, DataEnd, std::back_inserter(*m_mSessionBytes[vu8SourceIdentifier][SessionChunkType]));
    }
    else if (bLastInSequence && SameSesession && bSequenceContinuous)
    {
        // lets get the start and end of the data and store the bytes
        auto DataStart = pUDPChunk->m_vcDataChunk.begin() + pChunkHeaderState->m_uDataStartPosition;
        auto DataEnd = pUDPChunk->m_vcDataChunk.begin() + pChunkHeaderState->m_uDataStartPosition + pChunkHeaderState->m_puTransmissionSize.second;
        std::copy(DataStart, DataEnd, std::back_inserter(*m_mSessionBytes[vu8SourceIdentifier][SessionChunkType]));

        // Creating a TimeChunk into which data shall go
        auto pByteData = std::make_shared<std::vector<char>>(&(pUDPChunk->m_vcDataChunk));
        auto pBaseChunk = ChunkDuplicatorUtility::DeserialiseDerivedChunk(pByteData, SessionChunkType);

        // Pass pointer to data on and clear stored data and state information for current session
        TryPassChunk(pBaseChunk);
        m_mSessionModesStatesMap[vu8SourceIdentifier][SessionChunkType] = std::make_shared<ReliableSessionSessionMode>();
        m_mSessionBytes[vu8SourceIdentifier][SessionChunkType] = std::make_shared<std::vector<char>>();
    }
    else
    {
        std::cout << std::string(__FUNCTION__) + " - WAV session chunk missed, resetting [" + std::to_string(pChunkHeaderState->m_puSessionNumber.second) + ":" + std::to_string(pChunkHeaderState->m_puSequenceNumber.second) + "->" + std::to_string(pChunkHeaderState->m_uPreviousSequenceNumber) + "] \n";
        m_mSessionModesStatesMap[vu8SourceIdentifier][SessionChunkType] = std::make_shared<ReliableSessionSessionMode>();
        m_mSessionBytes[vu8SourceIdentifier][SessionChunkType] = std::make_shared<std::vector<char>>();
    }
}

std::shared_ptr<ReliableSessionSessionMode> SessionProcModule::GetPreviousSessionState(std::shared_ptr<BaseChunk> pBaseChunk, ChunkType chunkType)
{
    bool bProcessedBefore = false;
    auto pUDPChunk = std::static_pointer_cast<UDPChunk>(pBaseChunk);

    // If this unique identifier has been seen 
    if (m_mSessionModesStatesMap.count(pBaseChunk->GetSourceIdentifier()))
    {
        // And the chunk type has been processed before
        if (m_mSessionModesStatesMap[pBaseChunk->GetSourceIdentifier()].count(chunkType))
            // We can then return its session state
            return std::static_pointer_cast<ReliableSessionSessionMode>(m_mSessionModesStatesMap[pBaseChunk->GetSourceIdentifier()][chunkType]);
    }

    // If we have never seen this source identifier and chunk type 
    // then lets establish a session mode for it
    m_mSessionModesStatesMap[pBaseChunk->GetSourceIdentifier()][chunkType] = std::make_shared<ReliableSessionSessionMode>();
    return std::static_pointer_cast<ReliableSessionSessionMode>(m_mSessionModesStatesMap[pBaseChunk->GetSourceIdentifier()][chunkType]);
}
