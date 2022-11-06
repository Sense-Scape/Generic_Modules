#ifndef SIMULATORMODULE
#define SIMULATOR_MODULE

#include <map>
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
    void Process(std::shared_ptr<BaseChunk> pBaseChunk);

private:
    unsigned m_uNumChannels;                 ///< Number of ADC channels to simulate
    unsigned m_uSimulatedFrequency;          ///< Sinusoid frequency to simulate
    double m_dSampleRate;                    ///< Sample rate in Hz
    double m_dChunkSize;                     ///< How many samples in each chunk channel
    double m_dCurrentSampleIndex;            ///< What sample index is currently be created
    bool m_bSampleNow;                       ///< Whether ADC should sample now or wait
    bool m_bSamplingInProgress;              ///< If the module is already generating dataChunk
    bool m_bSamplingCompleted;               ///< boolean if the timechunk is full
    std::shared_ptr<TimeChunk> m_pTimeChunk; ///< Pointer to member time data chunk
    esp_timer_handle_t m_espPeriodicTimer;

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

    /**
     * @brief Starts ESP timer using SimulatorModule parameters
     *
     */
    void StartTimer();

    /**
     * @brief Function called everytime timer interrupt occurs. Infoms module to sample now
     * 
     * @param void_pbSampleNow 
     */
    static void SampleTimerCallback(void *void_pbSampleNow);
};

#endif