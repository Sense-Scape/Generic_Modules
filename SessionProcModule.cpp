#include "SessionProcModule.h"

SessionProcModule::SessionProcModule(unsigned uBufferSize) : BaseModule(uBufferSize),
                                                            m_mFunctionCallbacksMap(),
                                                            m_mSessionBytes()
{
    RegisterSessionStates();
    RegisterFunctionHandlers();
}

void SessionProcModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // find out what chunk type one is processing
    uint32_t u32ChunkType;
    auto pUDPChunk = std::static_pointer_cast<UDPChunk>(pBaseChunk);
    memcpy(&u32ChunkType, &pUDPChunk->m_vcDataChunk[9], sizeof(u32ChunkType));

    // Call a function as a function of chunk type
    m_mFunctionCallbacksMap[u32ChunkType](pUDPChunk);
}

void SessionProcModule::RegisterFunctionHandlers()
{
    // Function to call as a function of chunk type
    m_mFunctionCallbacksMap[ChunkTypesUtility::toU32(ChunkType::TimeChunk)] = [this](std::shared_ptr<BaseChunk> pBaseChunk) { ProcessTimeChunkSession(pBaseChunk); };
}

void SessionProcModule::RegisterSessionStates()
{
    // Map that stores session state as a function of session type
    m_mSessionModesStatesMap[SessionModeTypes::TimeChunkSession] = std::make_shared<TimeChunkSessionMode>();
}

void SessionProcModule::ProcessTimeChunkSession(std::shared_ptr<BaseChunk> pBaseChunk)
{
    //// TODO: Add ability to run sessions for each MAC Address

    // Extract state bytes and store in session state
    auto pTimeChunkHeaderState = std::dynamic_pointer_cast<TimeChunkSessionMode>(m_mSessionModesStatesMap[SessionModeTypes::TimeChunkSession]);
    auto pUDPChunk = std::dynamic_pointer_cast<UDPChunk>(pBaseChunk);
    pTimeChunkHeaderState->CovertBytesToStates(pUDPChunk);

    // Checking for sequence number continuity
    bool bStartSequence = (pTimeChunkHeaderState->m_puSequenceNumber.second == 0);
    bool bSequenceContinuous = (pTimeChunkHeaderState->m_puSequenceNumber.second == pTimeChunkHeaderState->m_uPreviousSequenceNumber + 1);
    bool bContinuingSequence = ((pTimeChunkHeaderState->m_pcTransmissionState.second == 0) && bSequenceContinuous);
    bool bLastInSequence = ((pTimeChunkHeaderState->m_pcTransmissionState.second == 1) && bSequenceContinuous);
    
    // Updating previous sequence after required for continuity/start checks
    pTimeChunkHeaderState->m_uPreviousSequenceNumber = pTimeChunkHeaderState->m_puSequenceNumber.second;

    // Store intermediate bytes
    if (bStartSequence || bContinuingSequence)
    {
        if (bStartSequence)
            m_mSessionBytes[SessionModeTypes::TimeChunkSession] = std::make_shared<std::vector<char>>();

        auto pvcIntermediateSessionBytes = m_mSessionBytes[SessionModeTypes::TimeChunkSession];
        auto DataStart = pUDPChunk->m_vcDataChunk.begin() + pTimeChunkHeaderState->m_uDataStartPosition;
        auto DataEnd = pUDPChunk->m_vcDataChunk.begin() + pTimeChunkHeaderState->m_puTransmissionSize.second;
        std::copy(DataStart, DataEnd, std::back_inserter(*pvcIntermediateSessionBytes));
    }
    else if (bLastInSequence)
    {
        auto pvcIntermediateSessionBytes = m_mSessionBytes[SessionModeTypes::TimeChunkSession];
        auto DataStart = pUDPChunk->m_vcDataChunk.begin() + pTimeChunkHeaderState->m_uDataStartPosition;
        auto DataEnd = pUDPChunk->m_vcDataChunk.begin() + pTimeChunkHeaderState->m_puTransmissionSize.second;

        std::copy(DataStart, DataEnd, std::back_inserter(*pvcIntermediateSessionBytes));

        auto len = pvcIntermediateSessionBytes->size();
        auto pTimeChunk = std::make_shared<TimeChunk>(0,0,0,0,0,0);
        pTimeChunk->Deserialise(pvcIntermediateSessionBytes);

        // Pass pointer to data on
        if (TryPassChunk(pTimeChunk))
            std::cout << std::string(__FUNCTION__) + " - WAV session complete passing WAV recording on \n";

        // Clear stored data and state information for current session
        m_mSessionModesStatesMap[SessionModeTypes::TimeChunkSession] = std::make_shared<TimeChunkSessionMode>();
        m_mSessionBytes[SessionModeTypes::TimeChunkSession] = std::make_shared<std::vector<char>>();
    }
    else
    {
        std::cout << std::string(__FUNCTION__) + " - WAV session chunk missed, resetting \n";
        m_mSessionModesStatesMap[SessionModeTypes::TimeChunkSession] = std::make_shared<TimeChunkSessionMode>();
        m_mSessionBytes[SessionModeTypes::TimeChunkSession] = std::make_shared<std::vector<char>>();
    }
}

