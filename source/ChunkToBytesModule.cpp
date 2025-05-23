#include "ChunkToBytesModule.h"

ChunkToBytesModule::ChunkToBytesModule(unsigned uBufferSize, unsigned uTransmissionSize) : BaseModule(uBufferSize),
                                                                                           m_uTransmissionSize(uTransmissionSize),
                                                                                           m_MapOfIndentifiersToChunkTypeSessions()
{
}

void ChunkToBytesModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    static uint8_t m_uSessionNumber = 0;

    // Lets first ensure that this chiunk is in the source identifers map
    auto vu8SourceIdentifier = pBaseChunk->GetSourceIdentifier();
    auto eChunkType = pBaseChunk->GetChunkType();

    bool bSourceIdentifierNotSeen = (m_MapOfIndentifiersToChunkTypeSessions.find(vu8SourceIdentifier) == m_MapOfIndentifiersToChunkTypeSessions.end());
    bool bChunkTypeNotSeen = true;

    if (!bSourceIdentifierNotSeen)
        bChunkTypeNotSeen = m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier].count(eChunkType) == 0;

    // By first checking if the source identider has been seen
    if (bSourceIdentifierNotSeen || bChunkTypeNotSeen)
    {
        m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier][eChunkType] = std::make_shared<SessionController>();
        m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier][eChunkType]->m_u32uChunkType = ChunkTypesNamingUtility::ToU32(eChunkType);

        for (size_t i = 0; i < vu8SourceIdentifier.size(); i++)
            m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier][eChunkType]->m_usUID[i] = vu8SourceIdentifier[i];
    }

    // Then we extract the current session state for the current chunk type and source identifier
    auto pSessionModeHeader = m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier][eChunkType];

    // PLOG_FATAL << std::to_string(pSessionModeHeader->m_u32uChunkType);
    //  Bytes to transmit is equal to number of bytes in derived object (e.g TimeChunk)
    auto pvcByteData = pBaseChunk->Serialise();
    u_int64_t u32TransmittableDataBytes = pvcByteData->size();

    uint16_t uSessionDataHeaderSize = 2; // size of the footer in bytes. '\0' denotes finish if this structure
    uint16_t uSessionTransmissionSize = m_uTransmissionSize;
    bool bTransmit = true;
    u_int64_t uDataBytesTransmitted = 0; // Current count of how many bytes have been transmitted
    uint16_t uDataBytesToTransmit = m_uTransmissionSize - uSessionDataHeaderSize - pSessionModeHeader->GetSize();
    uint16_t uMaxTransmissionSize = m_uTransmissionSize; // Largest buffer size that can be request for transmission

    // std::cout << std::to_string(u32TransmittableDataBytes) << std::endl;

    // Now that we have configured meta data, lets start transmitting
    while (bTransmit)
    {
        //std::cout << std::to_string(pSessionModeHeader->m_uSequenceNumber) + "--" + std::to_string(u32TransmittableDataBytes) << std::endl;
        // If our next transmission exceeds the number of data bytes available to be transmitted
        if (uDataBytesTransmitted + uDataBytesToTransmit >= u32TransmittableDataBytes)
        {
            // Then adjust to how many data bytes shall be transmitted to the remaining number
            uDataBytesToTransmit = u32TransmittableDataBytes - uDataBytesTransmitted;
            uSessionTransmissionSize = uDataBytesToTransmit + uSessionDataHeaderSize + pSessionModeHeader->GetSize();
            // And then inform process to finish up
            pSessionModeHeader->m_cTransmissionState = 1;
            bTransmit = false;
        }

        // Transmission Structure is shown below
        // { | DataHeaderSize | DatagramHeader | Data | }
        std::vector<char> vuByteData;
        vuByteData.resize(uSessionTransmissionSize);

        // Add in the transmission header
        memcpy(&vuByteData[0], &uSessionTransmissionSize, uSessionDataHeaderSize);

        // We then add the session state info
        auto pHeaderBytes = pSessionModeHeader->Serialise();
        
        memcpy(&vuByteData[uSessionDataHeaderSize], &((*pHeaderBytes)[0]), pSessionModeHeader->GetSize());

        // Then lets insert the actual data byte data to transmit after the header
        // While keeping in mind that we have to send unset bits from out data byte array
        memcpy(&vuByteData[uSessionDataHeaderSize + pSessionModeHeader->GetSize()], &((*pvcByteData)[uDataBytesTransmitted]), uDataBytesToTransmit);

        auto pByteChunk = std::make_shared<ByteChunk>(uSessionTransmissionSize);
        pByteChunk->m_vcDataChunk = vuByteData;
        pByteChunk->m_uChunkLength = uSessionTransmissionSize;

        TryPassChunk(pByteChunk);

        // Updating transmission states
        uDataBytesTransmitted += uDataBytesToTransmit;
        pSessionModeHeader->IncrementSequence();
    }
    
    m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier][eChunkType]->IncrementSession();
    
}

void ChunkToBytesModule::ContinuouslyTryProcess()
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