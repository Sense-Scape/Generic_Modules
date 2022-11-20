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
 *
 */
class SimulatorModule : public BaseModule
{

public:
    /**
     * @brief Construct a new ADCInterface object
     *
     * @param m_dSampleRate simulated sample rate in Hz
     * @param m_dChunkSize Number of sampels in a single channel of chunk data
     * @param uNumChannels Number of data channels to simulate
     * @param uSimulatedFrequency Frequency of sinusoid to simulate
     */
    SimulatorModule(double dSampleRate, double dChunkSize, unsigned uNumChannels, unsigned uSimulatedFrequency);
    ~SimulatorModule(){};

    /**
     * @brief Generate and fill complex time data chunk and pass on to next module
     *
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

	/**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::SimulatorModule; };

private:
    unsigned m_uNumChannels;                 ///< Number of ADC channels to simulate
    unsigned m_uSimulatedFrequency;          ///< Sinusoid frequency to simulate
    double m_dSampleRate;                    ///< Sample rate in Hz
    double m_dChunkSize;                     ///< How many samples in each chunk channel
    std::shared_ptr<TimeChunk> m_pTimeChunk; ///< Pointer to member time data chunk

    /**
     * @brief Initializes Time Chunk vectrs default values. Initializes according to number of ADCs and their channels
     *
     */
    void ReinitializeTimeChunk();

    /**
     * @brief Emulates Sampling of ADC. Stores a simulated sample accoriding to member sampling frequency
     *
     */
    void SimulateADCSample();

};

#endif