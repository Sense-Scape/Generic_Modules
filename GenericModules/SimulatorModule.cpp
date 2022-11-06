#include "SimulatorModule.h"

SimulatorModule::SimulatorModule(double dSampleRate, double dChunkSize, unsigned uNumChannels, unsigned uSimulatedFrequency) : BaseModule(),
                                                                                                                               m_uNumChannels(uNumChannels),
                                                                                                                               m_uSimulatedFrequency(uSimulatedFrequency),
                                                                                                                               m_dSampleRate(dSampleRate),
                                                                                                                               m_dChunkSize(dChunkSize),
                                                                                                                               m_dCurrentSampleIndex(0),
                                                                                                                               m_bSampleNow(),
                                                                                                                               m_bSamplingInProgress(),
                                                                                                                               m_bSamplingCompleted()

{
    std::cout << std::string(__PRETTY_FUNCTION__) + "  ADC module created with m_dSampleRate [ " + std::to_string(m_dSampleRate) + " ] Hz and m_dChunkSize [ " + std::to_string(m_dChunkSize) + " ] \n";
    m_pTimeChunk = std::make_shared<TimeChunk>(m_dChunkSize, m_dSampleRate, 0, 12, sizeof(float));
    StartTimer();
}

void SimulatorModule::Process()
{
    while (true)
    {
        if (!m_bSamplingInProgress)
            ReinitializeTimeChunk();

        SimulateADCSample();

        if (!m_bSamplingInProgress && m_bSamplingCompleted)
        {
            std::shared_ptr<TimeChunk> pTimeChunk = std::move(m_pTimeChunk);
            if (!TryPassChunk(std::dynamic_pointer_cast<BaseChunk>(pTimeChunk)))
            std::cout << std::string(__PRETTY_FUNCTION__) + ": Next buffer full, dropping current chunk and passing \n";
        }
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
    bool bUpdateIndex = false;

    if (m_bSampleNow)
    {
        // Iterating through each ADC channel vector
        for (unsigned uADCChannel = 0; uADCChannel < m_uNumChannels; uADCChannel++)
            m_pTimeChunk->m_vvvdTimeChunk[0][uADCChannel][m_dCurrentSampleIndex] = (float)sin(2 * 3.14159 * ((float)m_uSimulatedFrequency * m_dCurrentSampleIndex / (float)m_dSampleRate));

        m_bSampleNow = false;
        bUpdateIndex = true;
    }

    // If finished sampling chunk setting a bunch of flags
    if (m_dCurrentSampleIndex == m_dChunkSize)
    {
        m_bSamplingCompleted = true;
        m_bSamplingInProgress = false;
        m_dCurrentSampleIndex = 0;
        std::cout << std::string(__PRETTY_FUNCTION__) + " TimeChunk created and fully sampled \n";
    }
    else if (bUpdateIndex)
    {
        // Sampling not done
        m_bSampleNow = false;
        m_bSamplingInProgress = true;
        m_dCurrentSampleIndex++;
    }
}

void SimulatorModule::StartTimer()
{

    uint64_t uSamplePeriod = std::round(1e6 / m_dSampleRate);
    const esp_timer_create_args_t espPeriodicTimerArgs = {
        .callback = &SampleTimerCallback,
        .arg = (bool *)&m_bSampleNow};

    std::cout << std::string(__PRETTY_FUNCTION__) + " Starting timer with sampling period " + std::to_string(uSamplePeriod) + " us \n";

    ESP_ERROR_CHECK(esp_timer_create(&espPeriodicTimerArgs, &m_espPeriodicTimer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(m_espPeriodicTimer, uSamplePeriod));
}

void SimulatorModule::SampleTimerCallback(void *void_pbSampleNow)
{
    bool *pbSampleNow = (bool *)void_pbSampleNow;
    *pbSampleNow = true;
}
