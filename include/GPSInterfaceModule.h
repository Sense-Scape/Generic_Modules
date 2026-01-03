#ifndef GPS_INTERFACE_MODULE
#define GPS_INTERFACE_MODULE

/*Linux Includes*/
#include <fstream>
#include <gps.h>

/*Custom Includes*/
#include "BaseModule.h"

/**
 * @brief Class that reads the GPS data from the serial interface
 */
class GPSInterfaceModule : public BaseModule {
public:
  /**
   * @brief Constructor to communicate with serial GPS module
   * @param uBufferSize Size of module buffer
   * @param jsonConfig JSON configuration for the module
   */
  GPSInterfaceModule(unsigned uBufferSize,
                     nlohmann::json_abi_v3_11_2::json jsonConfig);

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

  const std::string m_strInterfaceName; ///< The string in the /dev directory
  const std::vector<uint8_t>
      m_vu8SourceIdentifier; ///< Source identifier of generated chunks
  // Simulation Data
  const bool m_bSimulateData;       ///< Whether GPS should try simulate Data
  std::fstream m_fsSerialInterface; ///< Stream to serial interface
  double m_dSimulatedLatitude = 0;  ///< The simulated latitide
  double m_dSimulatedLongitude = 0; ///< The simulated longitude
  bool m_bGPSCurrentlyLocked;       ///< Is the GPS currently locked
  gps_data_t gpsData;

private:
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
   * @brief Checks and logs if the GPS position has been set
   */
  void TryOpenGPSInterface();

  /**
   * @brief use a json object to configure this module
   * @param[in] jsonConfig JSON configuration of this module
   */
  void ConfigureModuleJSON(nlohmann::json_abi_v3_11_2::json jsonConfig);

  /**
   * @brief send reporting json messaages
   */
  void StartReportingLoop() override;
};

#endif
