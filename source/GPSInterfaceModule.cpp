#include "GPSInterfaceModule.h"

GPSInterfaceModule::GPSInterfaceModule(std::string strInterfaceName, std::vector<uint8_t> &vu8SourceIdentifier, unsigned uBufferSize, bool bSimulateData) : BaseModule(uBufferSize),
                                                                                                                                                            m_vu8SourceIdentifier(vu8SourceIdentifier),
                                                                                                                                                            m_strInterfaceName(strInterfaceName),
                                                                                                                                                            m_bSimulateData(bSimulateData)

{
    // If we dont simulate then try open an interface
    if (!bSimulateData)
        TryOpenSerialInterface();
}

void GPSInterfaceModule::ContinuouslyTryProcess()
{

    if (m_bSimulateData)
        CheckIfSimulationPositionSet();

    while (!m_bShutDown)
    {
        auto pBaseChunk = std::make_shared<BaseChunk>();
        Process(pBaseChunk);
    }
}

void GPSInterfaceModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    while (!m_bShutDown)
    {
        // Are connected
        if (m_bSimulateData)
        {
            TrySimulatedPositionData();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        else if (IsSerialInterfaceOpen())
        {
            TryTransmitPositionData();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        else
        {
            // If not then sleeop and try connect
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
            TryOpenSerialInterface();
        }
    }
}

bool GPSInterfaceModule::TryOpenSerialInterface()
{

    {
        std::string strInfo = "Opening interface to gps opened on: " + m_strInterfaceName;
        PLOG_INFO << strInfo;
    }

    m_fsSerialInterface = std::fstream(m_strInterfaceName.c_str());

    if (IsSerialInterfaceOpen())
    {
        std::string strInfo = "Serial interface to gps opened on: " + m_strInterfaceName;
        PLOG_INFO << strInfo;
        return true;
    }

    std::string strWarning = "Failed to open the serial port to GPS Module";
    PLOG_WARNING << strWarning;
    return false;
}

bool GPSInterfaceModule::IsSerialInterfaceOpen()
{
    return m_fsSerialInterface.is_open();
}

void GPSInterfaceModule::TryTransmitPositionData()
{
    if (m_fsSerialInterface.peek() != EOF)
    {

        // Read data from the serial port
        std::string strReceivedData;
        std::getline(m_fsSerialInterface, strReceivedData);

        // Print the received data
        if (strReceivedData.empty())
            return;

        // Extract and check the checksum provided in the sentence
        unsigned char ucExpectedChecksum = CalculateChecksum(strReceivedData);
        std::string strProvidedChecksumStr = strReceivedData.substr(strReceivedData.find('*') + 1, 2);
        unsigned char ucProvidedChecksum = std::stoul(strProvidedChecksumStr, nullptr, 16); // Convert hex string to unsigned char

        // Compare the calculated checksum with the provided checksum
        if (ucExpectedChecksum != ucProvidedChecksum)
        {
            std::string strWarning = "GPS Checksum is invalid: " + strReceivedData;
            PLOG_WARNING << strWarning;
            return;
        }

        // Now check it is position information
        std::string strGPSMessageType = strReceivedData.substr(strReceivedData.find('$') + 1, 5);
        if (strGPSMessageType != "GPGLL")
            return;

        // Read latitude, latitude direction, longitude, and longitude direction
        auto pGPSChunk = ExtractGSPData(strReceivedData);
        TryPassChunk(pGPSChunk);
    }
}

// Function to calculate the checksum for an NMEA sentence
unsigned char GPSInterfaceModule::CalculateChecksum(const std::string &sentence)
{
    unsigned char checksum = 0;

    // XOR all characters between '$' and '*' (excluding '$' and '*')
    for (size_t i = 1; i < sentence.length(); i++)
    {
        if (sentence[i] == '*')
            break;

        checksum ^= sentence[i];
    }

    return checksum;
}

std::shared_ptr<GPSChunk> GPSInterfaceModule::ExtractGSPData(const std::string &sentence)
{
    std::vector<std::string> result;
    size_t start = 0, end = 0;

    // split on commas to get all items in the string
    while (end != std::string::npos)
    {
        start = sentence.find_first_not_of(',', end);
        if (start == std::string::npos)
            break;

        end = sentence.find(',', start);
        if (end == std::string::npos)
        {
            result.push_back(sentence.substr(start));
            break;
        }

        result.push_back(sentence.substr(start, end - start));
    }

    auto pGPSChunk = std::make_shared<GPSChunk>();
    pGPSChunk->SetSourceIdentifier(m_vu8SourceIdentifier);

    // Get the current time point using system clock
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t epochTime = std::chrono::system_clock::to_time_t(now);
    pGPSChunk->m_i64TimeStamp = static_cast<uint64_t>(epochTime);

    pGPSChunk->m_bIsNorth = result[2] == "N";
    pGPSChunk->m_bIsWest = result[4] == "W";
    pGPSChunk->m_dLatitude = std::stod(result[1]);
    pGPSChunk->m_dLongitude = std::stod(result[3]);

    return pGPSChunk;
}

void GPSInterfaceModule::SetSimulationPosition(double dLong, double dLat)
{
    if (!m_bSimulateData)
    {
        PLOG_INFO << "GPS is in live mode";
        return;
    }

    if (dLong > 0)
        m_bSimulatedIsWest = true;
    else
        m_bSimulatedIsWest = false;
    m_dSimulatedLongitude = std::abs(dLong);

    if (dLat > 0)
        m_bSimulatedIsNorth = true;
    else
        m_bSimulatedIsNorth = false;
    m_dSimulatedLatitude = std::abs(dLat);

    PLOG_INFO << "GPS in simulated mode: long = " + std::to_string(dLong) + " lat = " + std::to_string(dLat);
}

void GPSInterfaceModule::TrySimulatedPositionData()
{

    auto pGPSChunk = std::make_shared<GPSChunk>();
    pGPSChunk->SetSourceIdentifier(m_vu8SourceIdentifier);

    // Get the current time point using system clock
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t epochTime = std::chrono::system_clock::to_time_t(now);
    pGPSChunk->m_i64TimeStamp = static_cast<uint64_t>(epochTime);

    pGPSChunk->m_bIsNorth = m_bSimulatedIsNorth;
    pGPSChunk->m_bIsWest = m_bSimulatedIsWest;
    pGPSChunk->m_dLatitude = m_dSimulatedLatitude;
    pGPSChunk->m_dLongitude = m_dSimulatedLongitude;

    TryPassChunk(pGPSChunk);
}

void GPSInterfaceModule::CheckIfSimulationPositionSet()
{
    if (!m_bSimulateData)
        return;

    if (m_dSimulatedLatitude == 0 && m_dSimulatedLongitude == 0)
        PLOG_WARNING << "GPS in simulation mode but no position has beens set";
}