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
    // Find out what chunk type one is processing
    uint32_t u32ChunkType;
    auto pUDPChunk = std::static_pointer_cast<UDPChunk>(pBaseChunk);
    memcpy(&u32ChunkType, &pUDPChunk->m_vcDataChunk[9], sizeof(u32ChunkType));

    // Call a function as a function of chunk type
    if(m_mFunctionCallbacksMap.count(u32ChunkType))
        m_mFunctionCallbacksMap[u32ChunkType](pUDPChunk);
    else
        std::cout << ChunkTypesUtility::toString(ChunkTypesUtility::FromU32(u32ChunkType)) + " not registered \n";
}

void SessionProcModule::RegisterFunctionHandlers()
{
    // Function to call as a function of chunk type
    m_mFunctionCallbacksMap[ChunkTypesUtility::ToU32(ChunkType::TimeChunk)] = [this](std::shared_ptr<BaseChunk> pBaseChunk) { ProcessTimeChunkSession(pBaseChunk); };
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
    std::shared_ptr<TimeChunkSessionMode> pTimeChunkHeaderState = std::static_pointer_cast<TimeChunkSessionMode>(m_mSessionModesStatesMap[SessionModeTypes::TimeChunkSession]);
    auto pUDPChunk = std::static_pointer_cast<UDPChunk>(pBaseChunk);
    pTimeChunkHeaderState->ConvertBytesToStates(pUDPChunk);

    // Checking all state variables
    // Are we the first message in a sequence ?
    bool bStartSequence = (pTimeChunkHeaderState->m_puSequenceNumber.second == 0);
    // If we are we then want to know if that sequence in continuous
    bool bSequenceContinuous = (pTimeChunkHeaderState->m_puSequenceNumber.second == pTimeChunkHeaderState->m_uPreviousSequenceNumber + 1); //intra
    // Now if we know about continuity, is this the last message in the sequence?
    bool bLastInSequence = pTimeChunkHeaderState->m_pcTransmissionState.second == 1 ? true : false;
    // Checking if this message belongs to the same sequences
    bool SameSesession = ((pTimeChunkHeaderState->m_puSessionNumber.second == pTimeChunkHeaderState->m_uPreviousSessionNumber) || pTimeChunkHeaderState->m_puSessionNumber.second == 0); //inter

    if (bStartSequence)
    {
        pTimeChunkHeaderState->m_uPreviousSessionNumber = pTimeChunkHeaderState->m_puSessionNumber.second;
    }
    // Updating previous sequence after required for continuity/start checks
    pTimeChunkHeaderState->m_uPreviousSequenceNumber = pTimeChunkHeaderState->m_puSequenceNumber.second;

    // we have just started or are continuing a seuqnce so store intermediate bytes
    if (bStartSequence || (SameSesession && !bLastInSequence && bSequenceContinuous))
    {
        // If this is the start create a vector to store data
        if (bStartSequence)
            m_mSessionBytes[SessionModeTypes::TimeChunkSession] = std::make_shared<std::vector<char>>();

        // lets get the start and end of the data and store the bytes
        auto DataStart = pUDPChunk->m_vcDataChunk.begin() + pTimeChunkHeaderState->m_uDataStartPosition;
        auto DataEnd = pUDPChunk->m_vcDataChunk.begin() + pTimeChunkHeaderState->m_uDataStartPosition + pTimeChunkHeaderState->m_puTransmissionSize.second;
        std::copy(DataStart, DataEnd, std::back_inserter(*m_mSessionBytes[SessionModeTypes::TimeChunkSession]));
    }
    else if (bLastInSequence && SameSesession && bSequenceContinuous)
    {
        // lets get the start and end of the data and store the bytes
        auto DataStart = pUDPChunk->m_vcDataChunk.begin() + pTimeChunkHeaderState->m_uDataStartPosition;
        auto DataEnd = pUDPChunk->m_vcDataChunk.begin() + pTimeChunkHeaderState->m_uDataStartPosition + pTimeChunkHeaderState->m_puTransmissionSize.second;
        std::copy(DataStart, DataEnd, std::back_inserter(*m_mSessionBytes[SessionModeTypes::TimeChunkSession]));

        // Creating a TimeChunk into which data shall go
        auto pTimeChunk = std::make_shared<TimeChunk>(0,0,0,0,0,0);
        pTimeChunk->Deserialise(m_mSessionBytes[SessionModeTypes::TimeChunkSession]);

        
        // Pass pointer to data on and clear stored data and state information for current session
        TryPassChunk(pTimeChunk);
        m_mSessionModesStatesMap[SessionModeTypes::TimeChunkSession] = std::make_shared<TimeChunkSessionMode>();
        m_mSessionBytes[SessionModeTypes::TimeChunkSession] = std::make_shared<std::vector<char>>();
    }
    else
    {
        std::cout << std::string(__FUNCTION__) + " - WAV session chunk missed, resetting [" + std::to_string(pTimeChunkHeaderState->m_puSessionNumber.second) + ":" + std::to_string(pTimeChunkHeaderState->m_puSequenceNumber.second) + "->" + std::to_string(pTimeChunkHeaderState->m_uPreviousSequenceNumber) + "] \n";
        m_mSessionModesStatesMap[SessionModeTypes::TimeChunkSession] = std::make_shared<TimeChunkSessionMode>();
        m_mSessionBytes[SessionModeTypes::TimeChunkSession] = std::make_shared<std::vector<char>>();
    }
}

