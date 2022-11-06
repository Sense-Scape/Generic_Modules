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
    auto pUDPChunk = std::static_pointer_cast<UDPChunk>(pBaseChunk);
    m_mFunctionCallbacksMap[SessionModeTypes::WAVSession](pUDPChunk);
}

void SessionProcModule::RegisterFunctionHandlers()
{
    m_mFunctionCallbacksMap[SessionModeTypes::WAVSession] = [this](std::shared_ptr<UDPChunk> pUDPChunk) { ProcessWAVSession(pUDPChunk); };
}

void SessionProcModule::RegisterSessionStates()
{
    m_mSessionModesStatesMap[SessionModeTypes::WAVSession] = std::make_shared<WAVSessionMode>();
}

void SessionProcModule::ProcessWAVSession(std::shared_ptr<UDPChunk> pUDPChunk)
{
    // TODO: Add ability to run sessions for each MAC Address
    // Updating session states
    auto pWAVHeaderState = std::static_pointer_cast<WAVSessionMode>(m_mSessionModesStatesMap[SessionModeTypes::WAVSession]);
    pWAVHeaderState->CovertBytesToStates(pUDPChunk);

    // Checking for sequence number continuity
    bool bStartSequence = (pWAVHeaderState->m_puSequenceNumber.second == 0);
    bool bContinuingSequence = ((pWAVHeaderState->m_pcTransmissionState.second == 0) && (pWAVHeaderState->m_puSequenceNumber.second == pWAVHeaderState->m_uPreviousSequenceNumber + 1));
    bool bLastInSequence = (pWAVHeaderState->m_pcTransmissionState.second == 1) && (pWAVHeaderState->m_puSequenceNumber.second == pWAVHeaderState->m_uPreviousSequenceNumber + 1);
    
    // Updating previous sequence after required for continuity/start checks
    pWAVHeaderState->m_uPreviousSequenceNumber = pWAVHeaderState->m_puSequenceNumber.second;

    // Store intermediate bytes
    if (bStartSequence || bContinuingSequence)
    {
        if (bStartSequence)
            m_mSessionBytes[SessionModeTypes::WAVSession] = std::make_shared<std::vector<char>>();

        auto pvcIntermediateSessionBytes = m_mSessionBytes[SessionModeTypes::WAVSession];
        std::copy(pUDPChunk->m_vcDataChunk.begin() + pWAVHeaderState->m_uDataStartPosition, pUDPChunk->m_vcDataChunk.begin() + pWAVHeaderState->m_puTransmissionSize.second, std::back_inserter(*pvcIntermediateSessionBytes));
    }
    else if (bLastInSequence)
    {
        auto pvcIntermediateSessionBytes = m_mSessionBytes[SessionModeTypes::WAVSession];
        std::copy(pUDPChunk->m_vcDataChunk.begin() + pWAVHeaderState->m_uDataStartPosition, pUDPChunk->m_vcDataChunk.begin() + pWAVHeaderState->m_puTransmissionSize.second, std::back_inserter(*pvcIntermediateSessionBytes));

        // Extracting WAV header and creating WAV chunk
        auto pWAVChunk = std::make_shared<WAVChunk>(pWAVHeaderState->m_pusMacUID.second);
        std::vector<char> vcWAVHeader(pvcIntermediateSessionBytes->begin(), pvcIntermediateSessionBytes->begin() + sizeof(WAVHeader));
        pWAVChunk->m_sWAVHeader = WAVChunk::BytesToWAVHeader(vcWAVHeader);

        // Converting remaining bytes to time samples
        std::vector<char> vcTimeData(pvcIntermediateSessionBytes->begin() + sizeof(WAVHeader), pvcIntermediateSessionBytes->end());
        unsigned sample = 0;
        while (sample < vcTimeData.size())
        {
            pWAVChunk->m_vfData.emplace_back(*(reinterpret_cast<float*>(&vcTimeData[sample])));
            sample += sizeof(float);
        }

        // Pass pointer to data on
        if (TryPassChunk(pWAVChunk))
            std::cout << std::string(__FUNCTION__) + " - WAV session complete passing WAV recording on \n";

        // Clear stored data and state information for current session
        m_mSessionModesStatesMap[SessionModeTypes::WAVSession] = std::make_shared<WAVSessionMode>();
        m_mSessionBytes[SessionModeTypes::WAVSession] = std::make_shared<std::vector<char>>();
    }
    else
    {
        std::cout << std::string(__FUNCTION__) + " - WAV session chunk missed, resetting \n";
        m_mSessionModesStatesMap[SessionModeTypes::WAVSession] = std::make_shared<WAVSessionMode>();
        m_mSessionBytes[SessionModeTypes::WAVSession] = std::make_shared<std::vector<char>>();
    }
}

