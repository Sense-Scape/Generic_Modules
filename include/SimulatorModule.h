#ifndef SIMULATORMODULE
#define SIMULATOR_MODULE

#include <map>
#include <chrono>
#include <cmath>
#include <thread>

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
     *
     * @param[in] dSampleRate simulated sample rate in Hz
     * @param[in] dChunkSize Number of sampels in a single channel of chunk data
     * @param[in] uNumChannels Number of data channels to simulate
     * @param[in] uSimulatedFrequency Frequency of sinusoid to simulate
     * @param[in] uBufferSize Size of input buffer
     */
    SimulatorModule(double dSampleRate, double dChunkSize, unsigned uNumChannels, unsigned uSimulatedFrequency, unsigned uBufferSize);
    //~SimulatorModule(){};

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

	/**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::SimulatorModule; };

    /**
     * @brief Check input buffer and try process data
     */
    void ContinuouslyTryProcess() override;

    /**
     * @brief Set the absolute phase (Rad) of each channel of simulated data
     * @param[in]
     */
    void SetChannelPhases(std::vector<float> &vfChannelPhases);


private:
    unsigned m_uNumChannels;                 ///< Number of ADC channels to simulate
    unsigned m_uSimulatedFrequency;          ///< Sinusoid frequency to simulate
    uint64_t m_uSampleCount;                 ///< Count to track index of simulated sinusoid
    double m_dSampleRate;                    ///< Sample rate in Hz
    double m_dChunkSize;                     ///< How many samples in each chunk channel
    std::shared_ptr<TimeChunk> m_pTimeChunk; ///< Pointer to member time data chunk
    std::vector<float> m_vfChannelPhases;    ///< Vector od channel phases

    /**
     * @brief Initializes Time Chunk vectors default values. Initializes according to number of ADCs and their channels
     */
    void ReinitializeTimeChunk();

    /**
     * @brief Emulates Sampling of ADC. Stores a simulated sample accoriding to member sampling frequency
     */
    void SimulateADCSample();

};

#endif