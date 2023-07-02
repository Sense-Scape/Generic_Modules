#include "ChunkToBytesModule.h"

ChunkToBytesModule::ChunkToBytesModule(unsigned uBufferSize, unsigned uTransmissionSize) :
	BaseModule(uBufferSize),
    m_uTransmissionSize(uTransmissionSize)
{

}

void ChunkToBytesModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    static uint8_t m_uSessionNumber = 0;

    // Bytes to transmit is equal to number of bytes in derived object (e.g TimeChunk)
    auto a = static_cast<TimeChunk&>(*pBaseChunk);
    auto pvcByteData = a.Serialise();
    uint32_t u32TransmittableDataBytes = pvcByteData->size();
    uint32_t u32ChunkType = ChunkTypesUtility::ToU32(pBaseChunk->GetChunkType());

    // Intra-transmission state information
    unsigned uDatagramHeaderSize = 24;      // NEEDS TO RESULT IN DATA WITH MULTIPLE OF 4 BYTES
    uint16_t uSessionDataHeaderSize = 2;          // size of the footer in bytes. '\0' denotes finish if this structure
    uint16_t uSessionTransmissionSize = m_uTransmissionSize;
    uint8_t uTransmissionState = 0;         // 0 - Transmitting; 1 - finished

    // Inter-transmission state information
    bool bTransmit = true;
    unsigned uSequenceNumber = 0;           // sequence 0 indicated error, 1 is starting
    unsigned uDataBytesTransmitted = 0;     // Current count of how many bytes have been transmitted
    unsigned uDataBytesToTransmit = m_uTransmissionSize - uSessionDataHeaderSize - uDatagramHeaderSize;
    unsigned uMaxTransmissionSize = m_uTransmissionSize; // Largest buffer size that can be request for transmission
    uint32_t uSessionNumber = m_uSessionNumber;

    // Now that we have configured meta data, lets start transmitting
    while (bTransmit)
    {
        // If our next transmission exceeds the number of data bytes available to be transmitted
        if (uDataBytesTransmitted + uDataBytesToTransmit > u32TransmittableDataBytes)
        {
            // Then adjust to how many data bytes shall be transmitted to the remaining number
            uDataBytesToTransmit = u32TransmittableDataBytes - uDataBytesTransmitted;
            uSessionTransmissionSize = uDataBytesToTransmit + uDatagramHeaderSize + uSessionDataHeaderSize;
            // And then inform process to finish up
            uTransmissionState = 1;
            bTransmit = false;
        }

        // Lets first insert the header transmission state information into the bytes array
        std::vector<char> vuUDPData;
        vuUDPData.resize(uSessionTransmissionSize);

        memcpy(&vuUDPData[0], &uSessionTransmissionSize, uSessionDataHeaderSize);
        memcpy(&vuUDPData[0 + uSessionDataHeaderSize], &uSequenceNumber, sizeof(uSequenceNumber)); // 4 bytes
        memcpy(&vuUDPData[4 + uSessionDataHeaderSize], &uTransmissionState, sizeof(uTransmissionState));
        memcpy(&vuUDPData[5 + uSessionDataHeaderSize], &uDataBytesToTransmit, sizeof(uDataBytesToTransmit));
        memcpy(&vuUDPData[9 + uSessionDataHeaderSize], &u32ChunkType, sizeof(u32ChunkType));
        memcpy(&vuUDPData[13 + uSessionDataHeaderSize], &uSessionNumber, sizeof(uSessionNumber));

        // Then lets insert the actual data byte data to transmit after the header
        // While keeping in mind that we have to send unset bits from out data byte array
        // { | DataHeaderSize | DatagramHeader | Data | }
        memcpy(&vuUDPData[uSessionDataHeaderSize + uDatagramHeaderSize], &((*pvcByteData)[uDataBytesTransmitted]), uDataBytesToTransmit);
        
        auto pUDPChunk = std::make_shared<UDPChunk>(uSessionTransmissionSize);
        pUDPChunk->m_vcDataChunk = vuUDPData;
        pUDPChunk->m_uChunkLength = uSessionTransmissionSize;
        TryPassChunk(pUDPChunk);

        // Updating transmission states
        uDataBytesTransmitted += uDataBytesToTransmit;
        uSequenceNumber++;
    }
    m_uSessionNumber++;
}