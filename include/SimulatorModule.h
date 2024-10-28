#ifndef SIMULATOR_MODULE
#define SIMULATOR_MODULE

#include <map>
#include <chrono>
#include <cmath>
#include <thread>
#include <random>

#define _USE_MATH_DEFINES // Allows the use of math constants
#include <math.h>

#include "BaseModule.h"
#include "TimeChunk.h"

/**
 * @brief Class that simulates and ADC sampling pure tones
 */
class SimulatorModule : public BaseModule
{
public:
    /**
     * @brief Construct a new ADCInterface object
     * @param[in] dSampleRate simulated sample rate in Hz
     * @param[in] dChunkSize Number of sampels in a single channel of chunk data
     * @param[in] uNumChannels Number of data channels to simulate
     * @param[in] uSimulatedFrequency Frequency of sinusoid to simulate
     * @param[in] uBufferSize Size of input buffer
     * @param[in] i64StartUpDelay_us start up delay in us of this simulator
     * @param[in] fSNR_db snr of generated signal
     */
    SimulatorModule(double dSampleRate, double dChunkSize, unsigned uNumChannels, unsigned uSimulatedFrequency, std::vector<uint8_t> &vu8SourceIdentifier, unsigned uBufferSize, int64_t i64StartUpDelay_us, float fSNR_db);
    //~SimulatorModule(){};

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

    /**
     * @brief Set the absolute phase (Rad) of each channel of simulated data
     * @param vfChannelPhases set of channel phases which to apply to channels
     * @param strPhaseType specify "Degree" otherwise assumed to be radian
     */
    void SetChannelPhases(std::vector<float> &vfChannelPhases, std::string strPhaseType);

private:
    /// Data Generation
    unsigned m_uNumChannels;                    ///< Number of ADC channels to simulate
    unsigned m_uSimulatedFrequency;             ///< Sinusoid frequency to simulate
    uint64_t m_u64SampleCount;                  ///< Count to track index of simulated sinusoid
    uint64_t m_u64CurrentTimeStamp;             ///< Simulated timestamp of the module
    double m_dSampleRate;                       ///< Sample rate in Hz
    double m_dChunkSize;                        ///< How many samples in each chunk channel
    std::shared_ptr<TimeChunk> m_pTimeChunk;    ///< Pointer to member time data chunk
    std::vector<float> m_vfChannelPhases_rad;   ///< Vector od channel phases
    std::vector<uint8_t> m_vu8SourceIdentifier; ///< Source identifier of generated chunks

    // Signal Generation
    uint32_t m_i64StartUpDelay_us;              ///< Delay in microseconds
    float m_fSNR_db;                            ///< SNR of signal
    float m_fSignalAmplitude;                   ///< As a percentage of full scale

    /**
     * @brief Initializes Time Chunk vectors default values. Initializes according to number of ADCs and their channels
     */
    void ReinitializeTimeChunk();

    /**
     * @brief Emulates Sampling of ADC. Stores a simulated sample accoriding to member sampling frequency
     */
    void SimulateADCSample();

    /**
     * @brief Simulates and updates change in timestamp as a function of sample rate and chunk size in microseconds
     */
    void SimulateUpdatedTimeStamp();

    /**
     * @brief Updates timechuink meta data including timestamp and source identifier
     */
    void SimulateTimeStampMetaData();

    /**
     * @brief Adds no gaussian white noise to generated signal using member noise levels
     */
    void AddNoiseToSamples();
};

#endif