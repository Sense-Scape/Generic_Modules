#include "SoundCardInterfaceModule.h"

SoundCardInterfaceModule::SoundCardInterfaceModule(double dSampleRate, double dChunkSize, unsigned uNumChannels, std::vector<uint8_t> &vu8SourceIdentifier, unsigned uBufferSize) : BaseModule(uBufferSize),
                                                                                                                                                                                    m_uNumChannels(uNumChannels),
                                                                                                                                                                                    m_dSampleRate(dSampleRate),
                                                                                                                                                                                    m_dChunkSize(dChunkSize),
                                                                                                                                                                                    m_vu8SourceIdentifier(vu8SourceIdentifier)
{

    std::string strSourceIdentifier = std::accumulate(m_vu8SourceIdentifier.begin(), m_vu8SourceIdentifier.end(), std::string(""),
                                                      [](std::string str, int element)
                                                      { return str + std::to_string(element) + " "; });
    std::string strInfo = std::string(__FUNCTION__) + " Simulator Module create:\n" + "=========\n" +
                          +"SourceIdentifier [ " + strSourceIdentifier + "] \n" + "SampleRate [ " + std::to_string(m_dSampleRate) + " ] Hz \n" + "ChunkSize [ " + std::to_string(m_dChunkSize) + " ]\n" + "=========";
    PLOG_INFO << strInfo;
    InitALSA();
}

void SoundCardInterfaceModule::ContinuouslyTryProcess()
{
    while (!m_bShutDown)
    {
        auto pBaseChunk = std::make_shared<BaseChunk>();
        Process(pBaseChunk);
    }
}

void SoundCardInterfaceModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Creating simulated data
    ReinitializeTimeChunk();

    if(!UpdatePCMSamples())
        return;

    UpdateTimeStampMetaData();
    UpdateTimeStamp(); 

    // Passing data on
    std::shared_ptr<TimeChunk> pTimeChunk = std::move(m_pTimeChunk);
    if (!TryPassChunk(std::static_pointer_cast<BaseChunk>(pTimeChunk)))
    {
        std::string strWarning = std::string(__FUNCTION__) + ": Next buffer full, dropping current chunk and passing \n";
        PLOG_WARNING << strWarning;
    }
}

void SoundCardInterfaceModule::ReinitializeTimeChunk()
{
    m_pTimeChunk = std::make_shared<TimeChunk>(m_dChunkSize, m_dSampleRate, 0, sizeof(int16_t) * 8, sizeof(int16_t), 1);
    m_pTimeChunk->m_vvi16TimeChunks.resize(m_uNumChannels);

    // Current implementation simulated N channels on a single ADC
    for (unsigned uChannel = 0; uChannel < m_uNumChannels; uChannel++)
        m_pTimeChunk->m_vvi16TimeChunks[uChannel].resize(m_dChunkSize);

    m_pTimeChunk->m_uNumChannels = m_uNumChannels;
}

bool SoundCardInterfaceModule::UpdatePCMSamples()
{
    int err;
    
    int iBufferFrames = m_dChunkSize;


    auto state = snd_pcm_state(m_capture_handle);
    if (state < 0) {
        fprintf(stderr, "Failed to get capture state: %s\n", snd_strerror(state));
        exit(1);
    }

    int frames_available = 0;

    while (frames_available <= iBufferFrames)
    {
        if (frames_available < 0) {
            fprintf(stderr, "Error getting available frames: %s\n", snd_strerror(frames_available));
            exit(1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        frames_available = snd_pcm_avail_update(m_capture_handle);
    } 

    if ((err = snd_pcm_readi(m_capture_handle, m_pvcAudioData->data(), iBufferFrames)) != iBufferFrames)
    {
        fprintf(stderr, "Read from audio interface failed (%d) (possible overrun): %s\n",
        err, snd_strerror(err));
        snd_pcm_prepare(m_capture_handle);
        snd_pcm_start(m_capture_handle);
        return false;
    }

    // Iterate through each sample capture
    int16_t *samples = (int16_t*)(m_pvcAudioData->data());
    for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < m_dChunkSize; uCurrentSampleIndex++)
    {
        // Then go through each channel per capture
        for (unsigned uCurrentChannelIndex = 0; uCurrentChannelIndex < m_uNumChannels; uCurrentChannelIndex++)
        {
            m_pTimeChunk->m_vvi16TimeChunks[uCurrentChannelIndex][uCurrentSampleIndex] = samples[uCurrentSampleIndex * m_uNumChannels + uCurrentChannelIndex];
        }
    }

    return true;
}

void SoundCardInterfaceModule::UpdateTimeStamp()
{
    // Get the current time point using system clock
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    // Convert the time point to duration since epoch
    std::time_t epochTime = std::chrono::system_clock::to_time_t(now);

    m_u64CurrentTimeStamp = static_cast<uint64_t>(epochTime);
}

void SoundCardInterfaceModule::UpdateTimeStampMetaData()
{
    m_pTimeChunk->m_i64TimeStamp = m_u64CurrentTimeStamp;
    m_pTimeChunk->SetSourceIdentifier(m_vu8SourceIdentifier);
}

void SoundCardInterfaceModule::InitALSA()
{
        int err;
        unsigned int uSampleRate = m_dSampleRate;

        // Specify the hardware device identifier
        const char *device = "hw:1,0";
        if ((err = snd_pcm_open(&m_capture_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
            PLOG_ERROR << "cannot open audio " + std::string(device) + " default (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "audio " + std::string(device) + " interface opened";

        if ((err = snd_pcm_hw_params_malloc(&m_hw_params)) < 0) {
            PLOG_ERROR << "cannot allocate hardware parameter structure (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "hw_params allocated";

        if ((err = snd_pcm_hw_params_any(m_capture_handle, m_hw_params)) < 0) {
            PLOG_ERROR << "cannot initialize hardware parameter structure (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "hw_params initialized";

        if ((err = snd_pcm_hw_params_set_access(m_capture_handle, m_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
            PLOG_ERROR << "cannot set access type (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "hw_params access set";

        if ((err = snd_pcm_hw_params_set_format(m_capture_handle, m_hw_params, m_format)) < 0) {
            PLOG_ERROR << "cannot set sample format (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "hw_params format set";

        if ((err = snd_pcm_hw_params_set_rate_near(m_capture_handle, m_hw_params, &uSampleRate, 0)) < 0) {
            PLOG_ERROR << "cannot set sample rate (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "hw_params rate set";

        if ((err = snd_pcm_hw_params_set_channels(m_capture_handle, m_hw_params, m_uNumChannels)) < 0) {
            PLOG_ERROR << "cannot set channel count (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "hw_params channels set";

        // Add these lines to set the buffer size
        snd_pcm_uframes_t period_size = (m_dChunkSize);  // Adjust this value as needed
        snd_pcm_uframes_t buffer_size = period_size*6;  // Adjust this value as needed

        if ((err = snd_pcm_hw_params_set_buffer_size_near(m_capture_handle, m_hw_params, &buffer_size)) < 0) {
            PLOG_ERROR << "cannot set buffer size (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "Buffer size set to " << buffer_size << " frames";

        if ((err = snd_pcm_hw_params_set_period_size_near(m_capture_handle, m_hw_params, &period_size, 0)) < 0) {
            PLOG_ERROR << "cannot set period size (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "Period size set to " << period_size << " frames";

        err = snd_pcm_nonblock(m_capture_handle, 1);
        if (err < 0) {
            fprintf(stderr, "Failed to set non-blocking mode: %s\n", snd_strerror(err));
            Cleanup();
            exit(1);
        }

        if ((err = snd_pcm_hw_params(m_capture_handle, m_hw_params)) < 0) {
            PLOG_ERROR << "cannot set parameters (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "hw_params set";

        snd_pcm_hw_params_free(m_hw_params);
        PLOG_INFO << "hw_params freed";

        if ((err = snd_pcm_prepare(m_capture_handle)) < 0) {
            PLOG_ERROR << "cannot prepare audio interface for use (" << snd_strerror(err) << ")";
            Cleanup();
            exit(1);
        }
        PLOG_INFO << "audio interface prepared";

         // Start the PCM
        if ((err = snd_pcm_start(m_capture_handle)) < 0) {
            fprintf(stderr, "Failed to start PCM: %s\n", snd_strerror(err));
            exit(1);
        }

        m_pTimeChunk = std::make_shared<TimeChunk>(m_dChunkSize, m_dSampleRate, 0, 12, sizeof(float), 1);
        m_pvcAudioData = std::make_shared<std::vector<char>>(m_dChunkSize * m_uNumChannels * snd_pcm_format_width(m_format) / 8);
    }

    void SoundCardInterfaceModule::Cleanup() {
        if (m_hw_params) {
            snd_pcm_hw_params_free(m_hw_params);
        }
        if (m_capture_handle) {
            snd_pcm_close(m_capture_handle);
        }
    }