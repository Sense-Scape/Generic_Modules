#ifndef SOUNDCARD_INTERFACE_MODULE
#define SOUNDCARD_INTERFACE_MODULE

/*Standard Includes*/
#include <chrono>
#include <cmath>
#include <ctime>
#include <map>
#include <thread>

#define _USE_MATH_DEFINES // Allows the use of math constants
#include <math.h>

/*Linux Includes*/
#include <alsa/asoundlib.h>

/*Custom Includes*/
#include "BaseModule.h"
#include "TimeChunk.h"

/**
 * @brief Class that reads pcm data from linux sound card
 */
class SoundCardInterfaceModule : public BaseModule {
public:
  /**
   * @brief Construct a new ADCInterface object
   * @param[in] uBufferSize Size of input buffer
   * @param[in] jsonConfig JSON configuration object
   */
  SoundCardInterfaceModule(unsigned uBufferSize,
                           nlohmann::json_abi_v3_11_2::json jsonConfig);

  /**
   * @brief Generate and fill complex time data chunk and pass on to next module
   */
  void Process(std::shared_ptr<BaseChunk> pBaseChunk);

  /**
   * @brief Returns module type
   * @param[out] ModuleType of processing module
   */
  std::string GetModuleType() override { return "SoundCardInterfaceModule"; };

  /**
   * @brief Check input buffer and try process data
   */
  void ContinuouslyTryProcess() override;

private:
  const unsigned m_uNumChannels; ///< Number of ADC channels to simulate
  const double m_dSampleRate;    ///< Sample rate in Hz
  const double m_dChunkSize;     ///< How many samples in each chunk channel
  const std::vector<uint8_t>
      m_vu8SourceIdentifier;      ///< Source identifier of generated chunks
  uint64_t m_u64CurrentTimeStamp; ///< Simulated timestamp of the module
  std::shared_ptr<TimeChunk>
      m_pTimeChunk; ///< Pointer to member time data chunk
  std::shared_ptr<std::vector<char>>
      m_pvcAudioData; ///< Pointer to vector to store audio data

  const char *m_pDevice = "hw:1,0";
  snd_pcm_t *m_capture_handle;
  snd_pcm_hw_params_t *m_hw_params;
  snd_pcm_format_t m_format = SND_PCM_FORMAT_S16_LE;

  /**
   * @brief Initializes Time Chunk vectors default values. Initializes according
   * to number of ADCs and their channels
   */
  void ReinitializeTimeChunk();

  /**
   * @brief Simulates and updates change in timestamp as a function of sample
   * rate and chunk size in microseconds
   */
  void SimulateUpdatedTimeStamp();

  /**
   * @brief Updates timechunk meta data including timestamp and source
   * identifier
   */
  void UpdateTimeStampMetaData();

  /**
   * @brief Updates timestamp to current clock time
   */
  void UpdateTimeStamp();

  /**
   * @brief Updates the current set of PCM samples to most recent from driver
   */
  bool UpdatePCMSamples();

  /**
   * @brief Configures the linux sound interface driver
   */
  void InitALSA();

  /**
   * @brief Clean Up alsa config
   */
  void Cleanup();

  /**
   * @brief send reporting json messaages
   */
  void StartReportingLoop() override;
};

#endif
