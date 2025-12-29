#ifndef SIMULATOR_MODULE
#define SIMULATOR_MODULE

#include <chrono>
#include <cmath>
#include <map>
#include <random>
#include <thread>

#define _USE_MATH_DEFINES // Allows the use of math constants
#include <math.h>

#include "BaseModule.h"
#include "TimeChunk.h"
#include "json.hpp"

/**
 * @brief Class that simulates and ADC sampling of tones or noise
 */
class SimulatorModule : public BaseModule {
public:
  /**
   * @brief Construct a new ADCInterface object
   * @param[in] jsonConfig
   */
  SimulatorModule(unsigned uBufferSize,
                  nlohmann::json_abi_v3_11_2::json jsonConfig);

  /**
   * @brief Generate and fill complex time data chunk and pass on to next module
   */
  void Process(std::shared_ptr<BaseChunk> pBaseChunk);

  /**
   * @brief Returns module type
   * @param[out] ModuleType of processing module
   */
  std::string GetModuleType() override { return "SimulatorModule"; };

  /**
   * @brief Check input buffer and try process data
   */
  void ContinuouslyTryProcess() override;

  /// Data Generation
  const unsigned m_uNumChannels;        ///< Number of ADC channels to simulate
  const unsigned m_uSimulatedFrequency; ///< Sinusoid frequency to simulate
  uint64_t m_u64SampleCount; ///< Count to track index of simulated sinusoid
  uint64_t m_u64CurrentTimeStamp_us; ///< Simulated timestamp of the module
  const double m_dSampleRate;        ///< Sample rate in Hz
  const double m_dChunkSize;         ///< How many samples in each chunk channel
  std::shared_ptr<TimeChunk>
      m_pTimeChunk; ///< Pointer to member time data chunk
  std::vector<float> m_vfChannelPhases_rad; ///< Vector of channel phases
  const std::vector<uint8_t>
      m_vu8SourceIdentifier; ///< Source identifier of generated chunks

  // Signal Generation
  const uint32_t m_i64StartUpDelay_us; ///< Delay in microseconds
  const float m_fSNR_db;               ///< SNR of signal
  const float m_fSignalPower_dBm;      ///< As a percentage of full scale
  float m_fAmplitudeScalingFactor;     ///< Power scaling facotr used when
                                       ///< generating a sinusoid

  const std::string m_strADCMode;   ///< Supporst either "Sunsoid" or "Gaussian"
  const std::string m_strClockMode; ///< Supports either "clock" or "Counter"

  std::mt19937 m_generator = std::mt19937(0);
  std::normal_distribution<double> m_dist;
  uint64_t m_count = 0;

  /**
   * @brief Initializes Time Chunk vectors default values. Initializes according
   * to number of ADCs and their channels
   */
  void ReinitializeTimeChunk();

  /**
   * @brief Emulates Sampling of ADC. Stores a simulated sample accoriding to
   * member sampling frequency
   */
  void SimulateADCSamples();

  /**
   * @brief Simulates and updates change in timestamp as a function of sample
   * rate and chunk size in microseconds
   */
  void SimulateUpdatedTimeStamp();

  /**
   * @brief Updates timechuink meta data including timestamp and source
   * identifier
   */
  void SimulateTimeStampMetaData();

  /**
   * @brief Adds no gaussian white noise to generated signal using member noise
   * levels
   */
  void SimulateNoise();

  /**
   * @brief Generate sinusoid at a fixed freqeuncy defined during class
   * construction
   */
  void GenerateSinsoid();

  /**
   * @brief Generate a signal which is just gaussian noise
   */
  void GenerateGaussianNoise();

  /**
   * @brief Set timestamp according to system clock at time of running thread
   */
  void GenerateClockTimestamp();

  /**
   * @brief Set timestamp and increment perfectly according to sample count,
   * starts from 0
   */
  void GenerateCountTimestamp();

  /**
   * @brief wait the specified amount of time before starting to process -
   * lmited to accuracy of thread sleep
   */
  void WaitForStartUpDelay();

  /**
   * @brief
   */
  float ConvertPowerToStdDev(float fSignalPower_dBm);

  /**
   * @brief Proint current configuration of this module
   */
  void PrintConfiguration();

  /**
   * @brief use a json object to configure this module
   * @param[in] jsonConfig JSON configuration of this module
   */
  void ConfigureModule(nlohmann::json_abi_v3_11_2::json jsonConfig);

  /**
   * @brief send reporting json messaages
   */
  void StartReportingLoop() override;

  template <typename T> double calculatePower(const std::vector<T> &signal) {
    double power =
        std::inner_product(signal.begin(), signal.end(), signal.begin(), 0.0) /
        signal.size();
    return power;
  }

  // Calculate power in dB (relative to reference level)
  template <typename T>
  double calculatePowerDB(const std::vector<T> &signal,
                          double referenceLevel = 1.0) {
    double power = calculatePower(signal);
    return 10.0 * std::log10(power / referenceLevel);
  }
};

#endif
