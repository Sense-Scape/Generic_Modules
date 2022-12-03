#include "SimulatorModule.h"

SimulatorModule::SimulatorModule(double dSampleRate, double dChunkSize, unsigned uNumChannels, unsigned uSimulatedFrequency, unsigned uBufferSize) : 
BaseModule(uBufferSize),                                                                                                                                                                                                                                            
m_uNumChannels(uNumChannels),
m_uSimulatedFrequency(uSimulatedFrequency),
m_dSampleRate(dSampleRate),
m_dChunkSize(dChunkSize)
{
    std::cout << std::string(__PRETTY_FUNCTION__) + "  ADC module created with m_dSampleRate [ " + std::to_string(m_dSampleRate) + " ] Hz and m_dChunkSize [ " + std::to_string(m_dChunkSize) + " ] \n";
    m_pTimeChunk = std::make_shared<TimeChunk>(m_dChunkSize, m_dSampleRate, 0, 12, sizeof(float));
}

void SimulatorModule::ContinuouslyTryProcess()
{
    std::unique_lock<std::mutex> ProcessLock(m_ProcessStateMutex);

    while (!m_bShutDown)
    {
        ProcessLock.unlock();
        auto pBaseChunk = std::make_shared<BaseChunk>();
        Process(pBaseChunk);

        ProcessLock.lock();
    }
}

void SimulatorModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    while (true)
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
        std::cout << std::string(__PRETTY_FUNCTION__) + ": sleeping for " + std::to_string((1000*m_dChunkSize)/m_dSampleRate) +" milliseconds \n";
		std::this_thread::sleep_for(std::chrono::milliseconds((unsigned)((1000*m_dChunkSize)/m_dSampleRate)));
    }
}

void SimulatorModule::ReinitializeTimeChunk()
{
    m_pTimeChunk = std::make_shared<TimeChunk>(m_dChunkSize, m_dSampleRate, 0, 12, sizeof(float));
    m_pTimeChunk->m_vvvdTimeChunk.resize(1);
    m_pTimeChunk->m_vvvdTimeChunk[0].resize(m_uNumChannels);

    unsigned uADCChannelCount = 0;

    // Current implementation simulated N channels on a single ADC
    for (unsigned uADCChannel = 0; uADCChannel < m_uNumChannels; uADCChannel++)
    {
        // Initialising channel data vector for each ADC
        m_pTimeChunk->m_vvvdTimeChunk[0][uADCChannel].resize(m_dChunkSize);
        uADCChannelCount++;
    }

    m_pTimeChunk->m_uNumChannels = m_uNumChannels;
}

void SimulatorModule::SimulateADCSample()
{
	// Iterating through each ADC channel vector and sample index
	for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < m_dChunkSize; uCurrentSampleIndex++)
	{
		for (unsigned uADCChannel = 0; uADCChannel < m_uNumChannels; uADCChannel++)
		{
			m_pTimeChunk->m_vvvdTimeChunk[0][uADCChannel][uCurrentSampleIndex] = (float)sin(2 * 3.14159 * ((float)m_uSimulatedFrequency * uCurrentSampleIndex / (float)m_dSampleRate));
		}
	}

    std::cout << std::string(__PRETTY_FUNCTION__) + " TimeChunk created and fully sampled \n";

}
