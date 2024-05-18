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
class GPSInterfaceModule : public BaseModule
{
public:
    /**
     * @brief Constructor to communicate with serial GPS module
     * @param strInterfaceName The interface with which communicate with GPS module
     * @param vu8SourceIdentifier Source identifier of chunks produced by this module
     * @param uBufferSize Size of module buffer
     * @param bSimulateData whether to simulate GPS data or not
     */
    GPSInterfaceModule(std::string strInterfaceName, std::vector<uint8_t> &vu8SourceIdentifier, unsigned uBufferSize, bool bSimulateData);

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    std::string GetModuleType() override { return "GPSInterfaceModule"; };

    /**
     * @brief Check input buffer and try process data
     */
    void ContinuouslyTryProcess() override;

    void SetSimulationPosition(double dLong, double dLat);

private:
    std::fstream m_fsSerialInterface;           ///< Stream to serial interface
    std::string m_strInterfaceName;             ///< The string in the /dev directory
    std::vector<uint8_t> m_vu8SourceIdentifier; ///< Source identifier of generated chunks
    // Simulation Data
    bool m_bSimulateData;           ///< Whether GPS should try simulate Data
    bool m_bSimulatedIsNorth;       ///< Whether the simualted position is north
    bool m_bSimulatedIsWest;        ///< Whether the simualted position is West
    bool m_dSimulatedLatitude = 0;  ///< The simulated latitide
    bool m_dSimulatedLongitude = 0; ///< The simulated longitude
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
     * @brief
     * @return
     */
    void TrySimulatedPositionData();

    /**
     * @brief
     * @return
     */
    void CheckIfSimulationPositionSet();

    /**
     * @brief Calaculates check sum of NMEA 0183 string
     * @return Check sum of the NMEA 0183 GPS string
     */
    unsigned char CalculateChecksum(const std::string &sentence);

    std::shared_ptr<GPSChunk> ExtractGSPData(const std::string &sentence);
};

#endif
