#include "GPSInterfaceModule.h"


GPSInterfaceModule::GPSInterfaceModule(unsigned uBufferSize, nlohmann::json_abi_v3_11_2::json jsonConfig) : 
    BaseModule(uBufferSize)
{
    ConfigureModuleJSON(jsonConfig);
    
    if(!m_bSimulateData)
        TryOpenGPSInterface();
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
        else
        {
            TryTransmitPositionData();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

void GPSInterfaceModule::TryOpenGPSInterface()
{
    while (!m_bShutDown)
    {
        if (gps_open("localhost", "2947", &gpsData) == -1) {
                PLOG_WARNING << "Failed to connect to gpsd";
                std::this_thread::sleep_for(std::chrono::milliseconds(10000));
            }
            else
            {
                PLOG_WARNING << "Connected to gpsd";
                gps_stream(&gpsData, WATCH_ENABLE, NULL);
                break;
            }
    }
}

void GPSInterfaceModule::TryTransmitPositionData()
{

    // Wait for a response
    char message[4096]; // Buffer for raw GPSD messages (optional)
    int message_len = sizeof(message); // Length of the buffer

    if (gps_waiting(&gpsData, 5000000)) { // Wait for up to 5 seconds

        if (gps_read(&gpsData, message, message_len) == -1) {
            PLOG_WARNING << "Failed to read from gpsd";
            gps_close(&gpsData);
            TryOpenGPSInterface();
        }

        auto pGPSChunk = std::make_shared<GPSChunk>();
        pGPSChunk->SetSourceIdentifier(m_vu8SourceIdentifier);

        // Get the current time point using system clock
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::time_t epochTime = std::chrono::system_clock::to_time_t(now);
        pGPSChunk->m_i64TimeStamp = static_cast<uint64_t>(epochTime);

        pGPSChunk->m_bIsNorth = gpsData.fix.latitude >= 0;
        pGPSChunk->m_bIsWest = gpsData.fix.longitude < 0;
        pGPSChunk->m_dLatitude = std::abs(gpsData.fix.latitude);
        pGPSChunk->m_dLongitude = std::abs(gpsData.fix.longitude);

        TryPassChunk(pGPSChunk);
    
    } 
    else
        PLOG_WARNING << "Timeout waiting for GPS data";
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
