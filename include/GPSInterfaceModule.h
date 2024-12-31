#ifndef GPS_INTERFACE_MODULE
#define GPS_INTERFACE_MODULE

/*Linux Includes*/
#include <iostream>
#include <fstream>

/*Custom Includes*/
#include "BaseModule.h"
#include "GPSChunk.h"

/**
 * @brief Class that reads the GPS data from the serial interface
 */
class   GPSInterfaceModule : public BaseModule
{
public:
    /**
     * @brief Constructor to communicate with serial GPS module
     * @param uBufferSize Size of module buffer
     * @param jsonConfig JSON configuration for the module
     */
    GPSInterfaceModule(unsigned uBufferSize, nlohmann::json_abi_v3_11_2::json jsonConfig);

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void DefaultProcess(std::shared_ptr<BaseChunk> pBaseChunk);

    /**
     * @brief Returns module type
     * @return ModuleType of processing module as a string
     */
    std::string GetModuleType() override { return "GPSInterfaceModule"; };

    /**
     * @brief Check input buffer and try process data
     */
    void ContinuouslyTryProcess() override;

    std::fstream m_fsSerialInterface;           ///< Stream to serial interface
    std::string m_strInterfaceName;             ///< The string in the /dev directory
    std::vector<uint8_t> m_vu8SourceIdentifier; ///< Source identifier of generated chunks
    // Simulation Data
    bool m_bSimulateData;             ///< Whether GPS should try simulate Data
    bool m_bSimulatedIsNorth;         ///< Whether the simualted position is north
    bool m_bSimulatedIsWest;          ///< Whether the simualted position is West
    double m_dSimulatedLatitude = 0;  ///< The simulated latitide
    double m_dSimulatedLongitude = 0; ///< The simulated longitude

private:
    /**
     * @brief Attempt to open a serial interace from the /dev directory
     * @return True or False as to whether the interface was opened
     */
    bool TryOpenSerialInterface();

    /**
     * @brief Checks if the module interface is open
     * @return True or False as to whether the interface is currently opened
     */
    bool IsSerialInterfaceOpen();

    /**
     * @brief Check serial interface of NMEA 0813 string fror posisiton data
     * @return True or False as to whether data was transmitted
     */
    void TryTransmitPositionData();

    /**
     * @brief Generates GPS chunk using simulated position
     */
    void TrySimulatedPositionData();

    /**
     * @brief Checks and logs if the GPS position has been set
     */
    void CheckIfSimulationPositionSet();

    /**
     * @brief Calaculates check sum of NMEA 0183 string
     * @return Check sum of the NMEA 0183 GPS string
     */
    unsigned char CalculateChecksum(const std::string &sentence);

    /**
     * @brief Extracts gps position from NMEA0813 from string
     * @param sentence string of GPS data
     * @return returns GPS chunk
     */
    std::shared_ptr<GPSChunk> ExtractGSPData(const std::string sentence);

    /**
     * @brief Verify the GPS string has valid data
     * @param sentence string of GPS data
     * @return returns GPS chunk
     */
    bool VerifyGPSData(const std::string strReceivedData);

    /**
     * @brief Print current configuration of this module
     */
    void PrintConfiguration();

    /**
     * @brief use a json object to configure this module
     * @param[in] jsonConfig JSON configuration of this module
     */
    void ConfigureModuleJSON(nlohmann::json_abi_v3_11_2::json jsonConfig);

    /**
     * @brief look for specified key and throw if not found
     * @param[in] jsonConfig JSON configuration of this module
     * @param[in] key Key for which one is looking
     */
    void CheckAndThrowJSON(const nlohmann::json_abi_v3_11_2::json& j, const std::string& key);
};

#endif
