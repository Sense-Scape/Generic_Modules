#include "GPSInterfaceModule.h"

GPSInterfaceModule::GPSInterfaceModule(unsigned uBufferSize, nlohmann::json_abi_v3_11_2::json jsonConfig) : 
    BaseModule(uBufferSize)
{
    ConfigureModuleJSON(jsonConfig);
    PrintConfiguration();
}

void GPSInterfaceModule::ConfigureModuleJSON(nlohmann::json_abi_v3_11_2::json jsonConfig)
{

    PLOG_INFO << jsonConfig.dump();
    
    CheckAndThrowJSON(jsonConfig, "SourceIdentifier");
    m_vu8SourceIdentifier = jsonConfig["SourceIdentifier"].get<std::vector<uint8_t>>();

    CheckAndThrowJSON(jsonConfig, "SerialPort");
    m_strInterfaceName = jsonConfig["SerialPort"];

    CheckAndThrowJSON(jsonConfig, "SimulatePosition");
    std::string strSimulatePosition = jsonConfig["SimulatePosition"];
    std::transform(strSimulatePosition.begin(), strSimulatePosition.end(), strSimulatePosition.begin(), 
                  [](unsigned char c) { return std::toupper(c); });
    m_bSimulateData = (strSimulatePosition == "TRUE");

    if (m_bSimulateData) {
        CheckAndThrowJSON(jsonConfig, "SimulatedLongitude");
        CheckAndThrowJSON(jsonConfig, "SimulatedLatitude");
        double dSimulatedLongitude = jsonConfig["SimulatedLongitude"];
        double dSimulatedLatitude = jsonConfig["SimulatedLatitude"];

        if (dSimulatedLongitude > 0)
            m_bSimulatedIsWest = false;
        else
            m_bSimulatedIsWest = true;
        m_dSimulatedLongitude = std::abs(dSimulatedLongitude);

        if (dSimulatedLatitude > 0)
            m_bSimulatedIsNorth = true;
        else
            m_bSimulatedIsNorth = false;
        m_dSimulatedLatitude = std::abs(dSimulatedLatitude);
    }
}

void GPSInterfaceModule::CheckAndThrowJSON(const nlohmann::json_abi_v3_11_2::json& j, const std::string& key) {
    auto it = j.find(key);
    if (it == j.end()) {
        std::string strFatal = std::string(__FUNCTION__) + "Key '" + key + "' not found in JSON.";
        PLOG_FATAL << strFatal;
        throw std::runtime_error(strFatal);
    }
}

void GPSInterfaceModule::PrintConfiguration()
{
    std::string strSourceIdentifier = std::accumulate(m_vu8SourceIdentifier.begin(), m_vu8SourceIdentifier.end(), std::string(""),
        [](std::string str, int element) { return str + std::to_string(element) + " "; });

    std::string strInfo = std::string(__FUNCTION__) + " GPS Module create:\n" 
        + "=========\n"
        + "General Config: \n"
        + "SourceIdentifier [ " + strSourceIdentifier + "] \n"
        + "SerialPort [ " + m_strInterfaceName + " ]\n"
        + "SimulatePosition [ " + (m_bSimulateData ? "TRUE" : "FALSE") + " ]\n";

    if (m_bSimulateData) {

        auto dLong  = m_dSimulatedLongitude * (m_bSimulatedIsWest ? -1 : 1);
        auto dLat  = m_dSimulatedLatitude * (m_bSimulatedIsNorth ? 1 : -1);

        strInfo += "Simulation Config: \n"
            + std::string("SimulatedLongitude [ " + std::to_string(dLong) + " ]\n")
            + std::string("SimulatedLatitude [ " + std::to_string(dLat) + " ]\n");
    }
    
    strInfo += "=========";
    PLOG_INFO << strInfo;
}

void GPSInterfaceModule::ContinuouslyTryProcess()
{

    if (m_bSimulateData)
        CheckIfSimulationPositionSet();

    while (!m_bShutDown)
    {
        auto pBaseChunk = std::make_shared<BaseChunk>();
        DefaultProcess(pBaseChunk);
    }
}

void GPSInterfaceModule::DefaultProcess(std::shared_ptr<BaseChunk> pBaseChunk)
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

        bool bGPSStringValid = VerifyGPSData(strReceivedData);
        if(!bGPSStringValid)
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

std::shared_ptr<GPSChunk> GPSInterfaceModule::ExtractGSPData(const std::string sentence)
{
    std::vector<std::string> result;
    size_t start = 0, end = 0;
    std::cout << sentence << std::endl;

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
    pGPSChunk->m_dLatitude = std::stod(result[1])/100.f;
    pGPSChunk->m_dLongitude = std::stod(result[3])/100.f;

    return pGPSChunk;
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

bool GPSInterfaceModule::VerifyGPSData(const std::string strReceivedData)
{
        // Print the received data
        if (strReceivedData.empty())
        {
            std::string strDebug = "GPS string is empty";
            PLOG_DEBUG << strDebug;
            return false;
        }

        // Extract and check the checksum provided in the sentence
        unsigned char ucExpectedChecksum = CalculateChecksum(strReceivedData);
        std::string strProvidedChecksumStr = strReceivedData.substr(strReceivedData.find('*') + 1, 2);
        unsigned char ucProvidedChecksum = std::stoul(strProvidedChecksumStr, nullptr, 16); // Convert hex string to unsigned char

        // Compare the calculated checksum with the provided checksum
        if (ucExpectedChecksum != ucProvidedChecksum)
        {
            std::string strDebug = "GPS Checksum is invalid: " + strReceivedData;
            PLOG_DEBUG << strDebug;
            return false;
        }

        // Now check it is position string   
        std::string strGPSMessageType = strReceivedData.substr(strReceivedData.find('$') + 1, 5);
        if (strGPSMessageType != "GPGLL" )
            return false;

        if (strGPSMessageType.size() <= 18)
        {
            std::string strDebug = "Position received with no position information - cant see satellite = " + strReceivedData;
            PLOG_DEBUG << strDebug;
            return false;
        }

        return true;
}