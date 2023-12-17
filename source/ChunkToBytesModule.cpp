#include "ChunkToBytesModule.h"


ChunkToBytesModule::ChunkToBytesModule(unsigned uBufferSize, unsigned uTransmissionSize) :
    BaseModule(uBufferSize),
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
    // By first checking if the source identider has been seen
    if (m_MapOfIndentifiersToChunkTypeSessions.find(vu8SourceIdentifier) == m_MapOfIndentifiersToChunkTypeSessions.end())
        // And then if the chunk type has been seen for this source
        if (m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier].find(eChunkType) ==
            m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier].end())
            // If not the create one
        {
            m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier][eChunkType] = std::make_shared<ReliableSessionSessionMode>();
            m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier][eChunkType]->m_u32uChunkType = ChunkTypesNamingUtility::ToU32(eChunkType);
            
            for (size_t i = 0; i < vu8SourceIdentifier.size(); i++)
            {
                m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier][eChunkType]->m_usUID[i] = vu8SourceIdentifier[i];
            }
        }

    // Then we extract the current session state for the current chunk type and source identifier
    auto pSessionModeHeader = m_MapOfIndentifiersToChunkTypeSessions[vu8SourceIdentifier][eChunkType];

    // Bytes to transmit is equal to number of bytes in derived object (e.g TimeChunk)
    auto pvcByteData = pBaseChunk->Serialise();
    uint32_t u32TransmittableDataBytes = pvcByteData->size();


    uint16_t uSessionDataHeaderSize = 2;          // size of the footer in bytes. '\0' denotes finish if this structure
    uint16_t uSessionTransmissionSize = m_uTransmissionSize;
    bool bTransmit = true;
    unsigned uDataBytesTransmitted = 0;     // Current count of how many bytes have been transmitted
    unsigned uDataBytesToTransmit = m_uTransmissionSize - uSessionDataHeaderSize - pSessionModeHeader->GetSize();
    unsigned uMaxTransmissionSize = m_uTransmissionSize; // Largest buffer size that can be request for transmission

    // Now that we have configured meta data, lets start transmitting
    while (bTransmit)
    {
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
        std::vector<char> vuUDPData;
        vuUDPData.resize(uSessionTransmissionSize);

        // Add in the transmission header
        memcpy(&vuUDPData[0], &uSessionTransmissionSize, uSessionDataHeaderSize);

        // We then add the session state info
        auto pHeaderBytes = pSessionModeHeader->Serialise();
        memcpy(&vuUDPData[uSessionDataHeaderSize], &((*pHeaderBytes)[0]), pSessionModeHeader->GetSize());


        // Then lets insert the actual data byte data to transmit after the header
        // While keeping in mind that we have to send unset bits from out data byte array
        memcpy(&vuUDPData[uSessionDataHeaderSize + pSessionModeHeader->GetSize()], &((*pvcByteData)[uDataBytesTransmitted]), uDataBytesToTransmit);

        auto pUDPChunk = std::make_shared<UDPChunk>(uSessionTransmissionSize);
        pUDPChunk->m_vcDataChunk = vuUDPData;
        pUDPChunk->m_uChunkLength = uSessionTransmissionSize;

        TryPassChunk(pUDPChunk);

        // Updating transmission states
        uDataBytesTransmitted += uDataBytesToTransmit;
        pSessionModeHeader->IncrementSequence();
    }
    pSessionModeHeader->IncrementSession();
}