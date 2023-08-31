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

    uint16_t uSessionTransmissionSize;
    memcpy(&uSessionTransmissionSize, &pUDPChunk->m_vcDataChunk[0], sizeof(uSessionTransmissionSize));

    auto pvcByteData = std::make_shared<std::vector<char>>(pUDPChunk->m_vcDataChunk.begin() + 2, pUDPChunk->m_vcDataChunk.end());
    auto pChunkHeaderState = std::make_shared<ReliableSessionSessionMode>();
    pChunkHeaderState->Deserialise(pvcByteData);

    // Then we can map keys
    ChunkType SessionChunkType = ChunkTypesNamingUtility::FromU32(pChunkHeaderState->m_u32uChunkType);
    auto vu8SourceIdentifier = pUDPChunk->GetSourceIdentifier();

    auto  pPreviousChunkHeaderState = GetPreviousSessionState(vu8SourceIdentifier, SessionChunkType);


    // Now we can check all state variables
    // Are we the first message in a sequence ?
    bool bStartSequence = (pChunkHeaderState->m_uSequenceNumber == 0);
    // If we are we then want to know if that sequence in continuous
    bool bSequenceContinuous = (pChunkHeaderState->m_uSequenceNumber == pPreviousChunkHeaderState->m_uSequenceNumber + 1); //intra
    // Now if we know about continuity, is this the last message in the sequence?
    bool bLastInSequence = pChunkHeaderState->m_cTransmissionState == 1 ? true : false;
    // Checking if this message belongs to the same sequences
    bool SameSesession = ((pChunkHeaderState->m_uSessionNumber == pPreviousChunkHeaderState->m_uSessionNumber) || pChunkHeaderState->m_uSessionNumber == 0); //inter

    if (bStartSequence)
        pPreviousChunkHeaderState->m_uSessionNumber = pChunkHeaderState->m_uSessionNumber;

    // Updating previous sequence after required for continuity/start checks
    pPreviousChunkHeaderState->m_uSequenceNumber = pChunkHeaderState->m_uSequenceNumber;

    // We have just started or are continuing a sequence so store intermediate bytes
    if (bStartSequence || (SameSesession && !bLastInSequence && bSequenceContinuous))
    {
        // If this is the start create a vector to store data
        if (bStartSequence)
            m_mSessionBytes[vu8SourceIdentifier][SessionChunkType] = std::make_shared<std::vector<char>>();

        // lets get the start and end of the data and store the bytes
        auto DataStart = pUDPChunk->m_vcDataChunk.begin() + pChunkHeaderState->GetSize();
        auto DataEnd = pUDPChunk->m_vcDataChunk.begin() + pChunkHeaderState->GetSize() + uSessionTransmissionSize;
        std::copy(DataStart, DataEnd, std::back_inserter(*m_mSessionBytes[vu8SourceIdentifier][SessionChunkType]));
    }
    else if (bLastInSequence && SameSesession && bSequenceContinuous)
    {
        // lets get the start and end of the data and store the bytes
        auto DataStart = pUDPChunk->m_vcDataChunk.begin() + pChunkHeaderState->GetSize();
        auto DataEnd = pUDPChunk->m_vcDataChunk.begin() + pChunkHeaderState->GetSize() + uSessionTransmissionSize; 
        std::copy(DataStart, DataEnd, std::back_inserter(*m_mSessionBytes[vu8SourceIdentifier][SessionChunkType]));

        // Creating a TimeChunk into which data shall go
        auto pByteData = std::make_shared<std::vector<char>>(pUDPChunk->m_vcDataChunk);
        auto pBaseChunk = ChunkDuplicatorUtility::DeserialiseDerivedChunk(pByteData, SessionChunkType);

        // Pass pointer to data on and clear stored data and state information for current session
        TryPassChunk(pBaseChunk);
        m_mSessionModesStatesMap[vu8SourceIdentifier][SessionChunkType] = std::make_shared<ReliableSessionSessionMode>();
        m_mSessionBytes[vu8SourceIdentifier][SessionChunkType] = std::make_shared<std::vector<char>>();
    }
    else
    {
        std::cout << std::string(__FUNCTION__) + " - WAV session chunk missed, resetting [" + std::to_string(pChunkHeaderState->m_uSessionNumber) + ":" + std::to_string(pChunkHeaderState->m_uSequenceNumber) + "->" + std::to_string(pPreviousChunkHeaderState->m_uSequenceNumber) + "] \n";
        pChunkHeaderState = std::make_shared<ReliableSessionSessionMode>();
        m_mSessionBytes[vu8SourceIdentifier][SessionChunkType] = std::make_shared<std::vector<char>>();
    }

    UpdatePreviousSessionState(vu8SourceIdentifier, SessionChunkType, *pChunkHeaderState);
}

std::shared_ptr<ReliableSessionSessionMode> SessionProcModule::GetPreviousSessionState(std::vector<uint8_t>& vu8SourceIdentifier, ChunkType chunkType)
{
    bool bProcessedBefore = false;

    // If this unique identifier has been seen 
    if (m_mSessionModesStatesMap.count(vu8SourceIdentifier))
    {
        // And the chunk type has been processed before
        if (m_mSessionModesStatesMap[vu8SourceIdentifier].count(chunkType))
            // We can then return its session state
            return std::static_pointer_cast<ReliableSessionSessionMode>(m_mSessionModesStatesMap[vu8SourceIdentifier][chunkType]);
    }

    // If we have never seen this source identifier and chunk type 
    // then lets establish a session mode for it
    m_mSessionModesStatesMap[vu8SourceIdentifier][chunkType] = std::make_shared<ReliableSessionSessionMode>();
    return std::static_pointer_cast<ReliableSessionSessionMode>(m_mSessionModesStatesMap[vu8SourceIdentifier][chunkType]);
}

void SessionProcModule::UpdatePreviousSessionState(std::vector<uint8_t>& vu8SourceIdentifier, ChunkType chunkType, ReliableSessionSessionMode reliableSessionMode)
{
    m_mSessionModesStatesMap[vu8SourceIdentifier][chunkType] = std::make_shared<ReliableSessionSessionMode>(reliableSessionMode);
}