#include "SimulatorModule.h"
#include "esp_system.h"

SimulatorModule::SimulatorModule(double dSampleRate, double dChunkSize, unsigned uNumChannels, unsigned uSimulatedFrequency, unsigned uBufferSize) : 
BaseModule(uBufferSize),                                                                                                                                                                                                                                            
m_uNumChannels(uNumChannels),
m_uSimulatedFrequency(uSimulatedFrequency),
m_dSampleRate(dSampleRate),
m_dChunkSize(dChunkSize)
{
    // Set all channel phases to zero with angle of arrival = 0 
    m_vfChannelPhases.resize(m_uNumChannels);

    std::cout << std::string(__PRETTY_FUNCTION__) + "  ADC module created with m_dSampleRate [ " + std::to_string(m_dSampleRate) + " ] Hz and m_dChunkSize [ " + std::to_string(m_dChunkSize) + " ] \n";
    m_pTimeChunk = std::make_shared<TimeChunk>(m_dChunkSize, m_dSampleRate, 0, 12, sizeof(float),1);
}

void SimulatorModule::ContinuouslyTryProcess()
{
    while (!m_bShutDown)
    {
        auto pBaseChunk = std::make_shared<BaseChunk>();
        Process(pBaseChunk);
    }
}

void SimulatorModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Creating simulated data
    ReinitializeTimeChunk();
    SimulateADCSample();
    
    // Passing data on
    std::shared_ptr<TimeChunk> pTimeChunk = std::move(m_pTimeChunk);
    if (!TryPassChunk(std::static_pointer_cast<BaseChunk>(pTimeChunk)))
    {
        std::cout << std::string(__PRETTY_FUNCTION__) + ": Next buffer full, dropping current chunk and passing \n";
    }

    // Sleeping for time equivalent to chunk period
    std::this_thread::sleep_for(std::chrono::milliseconds((unsigned)((1000*m_dChunkSize)/m_dSampleRate)));
}

void SimulatorModule::ReinitializeTimeChunk()
{
    m_pTimeChunk = std::make_shared<TimeChunk>(m_dChunkSize, m_dSampleRate, 0, sizeof(int16_t)*8, sizeof(int16_t), 1);
    m_pTimeChunk->m_vvi16TimeChunks.resize(m_uNumChannels);

    // Current implementation simulated N channels on a single ADC
    for (unsigned uChannel = 0; uChannel < m_uNumChannels; uChannel++)
    {
        // Initialising channel data vector for each ADC
        m_pTimeChunk->m_vvi16TimeChunks[uChannel].resize(m_dChunkSize);
    }

    m_pTimeChunk->m_uNumChannels = m_uNumChannels;
}

void SimulatorModule::SimulateADCSample()
{
	// Iterate through each channel vector and sample index
	for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < m_dChunkSize; uCurrentSampleIndex++)
	{
		for (unsigned uChannel = 0; uChannel < m_uNumChannels; uChannel++)
		{
            // And create a "sampled" float value between 0 and 1 with phase offests
            auto datum = sin(2 * 3.14159 * ((float)m_uSimulatedFrequency * uCurrentSampleIndex / (float)m_dSampleRate) + m_vfChannelPhases[uChannel]);
			// The take this value, scale and convert it to a signed int
            m_pTimeChunk->m_vvi16TimeChunks[uChannel][uCurrentSampleIndex] = (int16_t)(datum*std::pow(2,15));
		}
	}
}

void SimulatorModule::SetChannelPhases(std::vector<float> &vfChannelPhases)
{
    m_vfChannelPhases = vfChannelPhases;
}
