#include "SimulatorModule.h"

SimulatorModule::SimulatorModule(double dSampleRate, double dChunkSize, unsigned uNumChannels, unsigned uSimulatedFrequency, std::vector<uint8_t> &vu8SourceIdentifier, unsigned uBufferSize,  int64_t i64StartUpDelay_us) : 
BaseModule(uBufferSize),                                                                                                                                                                                                                                            
m_uNumChannels(uNumChannels),
m_u64SampleCount(0),
m_uSimulatedFrequency(uSimulatedFrequency),
m_dSampleRate(dSampleRate),
m_dChunkSize(dChunkSize),
m_vu8SourceIdentifier(vu8SourceIdentifier),
m_i64StartUpDelay_us(i64StartUpDelay_us)
{
    // Set all channel phases to zero with angle of arrival = 0 
    m_vfChannelPhases_rad.resize(m_uNumChannels);

    std::string strSourceIdentifier = std::accumulate(m_vu8SourceIdentifier.begin(), m_vu8SourceIdentifier.end(), std::string(""),
        [](std::string str, int element) { return str + std::to_string(element) + " "; });
    std::string strInfo = std::string(__FUNCTION__) + " Simulator Module create:\n" 
        + "=========\n" +
        + "SourceIdentifier [ " + strSourceIdentifier + "] \n" 
        + "SampleRate [ " + std::to_string(m_dSampleRate) + " ] Hz \n"
        + "ChunkSize [ " + std::to_string(m_dChunkSize) + " ]\n" 
        + "Start Up Delay (us) [ " + std::to_string(m_i64StartUpDelay_us) + " ]\n" 
        + "Number of Channels [ " + std::to_string(m_uNumChannels) + " ]\n" 
        + "=========";
    PLOG_INFO << strInfo;

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
    SimulateTimeStampMetaData();
    SimulateUpdatedTimeStamp();
    
    // Passing data on
    std::shared_ptr<TimeChunk> pTimeChunk = std::move(m_pTimeChunk);
    if (!TryPassChunk(std::static_pointer_cast<BaseChunk>(pTimeChunk)))
    {
        std::string strWarning = std::string(__FUNCTION__) + ": Next buffer full, dropping current chunk and passing \n";
        PLOG_WARNING << strWarning;
    }

    // Sleeping for time equivalent to chunk period
    std::this_thread::sleep_for(std::chrono::nanoseconds((unsigned)((1e9*m_dChunkSize)/m_dSampleRate)));
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
            int16_t datum = (std::pow(2, 15) - 1)* sin((double)2.0 * M_PI * ((double)m_uSimulatedFrequency * (double)m_u64SampleCount / (double)m_dSampleRate) + (double)m_vfChannelPhases_rad[uChannel]);
            // The take this value, scale and convert it to a signed int
            m_pTimeChunk->m_vvi16TimeChunks[uChannel][uCurrentSampleIndex] = datum;
        }

        // Now lets check if we are at the end of the
        // sinusoid and have to reset the counter
        m_u64SampleCount += 1;   
	}
}

void SimulatorModule::SetChannelPhases(std::vector<float> &vfChannelPhases, std::string strPhaseType)
{
   

    if (strPhaseType == "Degrees")
    {
        for (size_t i = 0; i < vfChannelPhases.size(); i++)
            vfChannelPhases[i] = vfChannelPhases[i] * M_PI/180.0;  
    }

    m_vfChannelPhases_rad = vfChannelPhases;

    PLOG_INFO << "Channel Phases: " << m_vfChannelPhases_rad[0];

}

void SimulatorModule::SimulateUpdatedTimeStamp()
{   
    auto now = std::chrono::system_clock::now();
    auto i64CurrentTime_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    m_u64CurrentTimeStamp = i64CurrentTime_us + m_i64StartUpDelay_us; 
}

void SimulatorModule::SimulateTimeStampMetaData()
{
    m_pTimeChunk->m_i64TimeStamp = m_u64CurrentTimeStamp;
    m_pTimeChunk->SetSourceIdentifier(m_vu8SourceIdentifier);
}