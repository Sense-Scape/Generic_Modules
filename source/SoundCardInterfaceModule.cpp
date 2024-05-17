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

    m_pTimeChunk = std::make_shared<TimeChunk>(m_dChunkSize, m_dSampleRate, 0, 12, sizeof(float), 1);

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
    UpdatePCMSamples();
    UpdateTimeStampMetaData();
    UpdateTimeStamp();

    // Passing data on
    std::shared_ptr<TimeChunk> pTimeChunk = std::move(m_pTimeChunk);
    if (!TryPassChunk(std::static_pointer_cast<BaseChunk>(pTimeChunk)))
    {
        std::string strWarning = std::string(__FUNCTION__) + ": Next buffer full, dropping current chunk and passing \n";
        PLOG_WARNING << strWarning;
    }

    // Sleeping for time equivalent to chunk period
    std::this_thread::sleep_for(std::chrono::milliseconds((unsigned)((1000 * m_dChunkSize) / m_dSampleRate)));
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

void SoundCardInterfaceModule::UpdatePCMSamples()
{

    int i;
    int err;
    char *pcBuffer;
    int iBufferFrames = 512;

    pcBuffer = (char *)malloc(512 * snd_pcm_format_width(m_format) / 8 * 4);
    if (pcBuffer == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for buffer\n");
        exit(1);
    }

    err = snd_pcm_wait(m_capture_handle, -1);
    if ((err = snd_pcm_readi(m_capture_handle, pcBuffer, iBufferFrames)) != iBufferFrames)
    {
        fprintf(stderr, "Read from audio interface failed (%d): %s\n",
                err, snd_strerror(err));
        free(pcBuffer);
        exit(1);
    }

    for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < m_dChunkSize; uCurrentSampleIndex++)
    {
        for (unsigned uCurrentChannelIndex = 0; uCurrentChannelIndex < m_uNumChannels; uCurrentChannelIndex++)
        {
            unsigned uBufferOffset = uCurrentSampleIndex * m_uNumChannels * 2;
            unsigned uChannelOffset = 2 * uCurrentChannelIndex;
            m_pTimeChunk->m_vvi16TimeChunks[uCurrentChannelIndex][uCurrentSampleIndex] = ((*pcBuffer + uBufferOffset + uChannelOffset) | ((*(pcBuffer + uBufferOffset + (uChannelOffset + 1))) << 8));
        }
    }

    free(pcBuffer);
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

    if ((err = snd_pcm_open(&m_capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        PLOG_ERROR << "cannot open audio device default (" << snd_strerror(err) << ")";
        exit(1);
    }

    PLOG_INFO << "audio interface opened";

    if ((err = snd_pcm_hw_params_malloc(&m_hw_params)) < 0)
    {
        PLOG_ERROR << "cannot allocate hardware parameter structure (" << snd_strerror(err) << ")";
        exit(1);
    }

    PLOG_INFO << "hw_params allocated";

    if ((err = snd_pcm_hw_params_any(m_capture_handle, m_hw_params)) < 0)
    {
        PLOG_ERROR << "cannot initialize hardware parameter structure (" << snd_strerror(err) << ")";
        exit(1);
    }

    PLOG_INFO << "hw_params initialized";

    if ((err = snd_pcm_hw_params_set_access(m_capture_handle, m_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        PLOG_ERROR << "cannot set access type (" << snd_strerror(err) << ")";
        exit(1);
    }

    PLOG_INFO << "hw_params access set";

    if ((err = snd_pcm_hw_params_set_format(m_capture_handle, m_hw_params, m_format)) < 0)
    {
        PLOG_ERROR << "cannot set sample format (" << snd_strerror(err) << ")";
        exit(1);
    }

    PLOG_INFO << "hw_params format set";

    if ((err = snd_pcm_hw_params_set_rate_near(m_capture_handle, m_hw_params, &(uSampleRate), 0)) < 0)
    {
        PLOG_ERROR << "cannot set sample rate (" << snd_strerror(err) << ")";
        exit(1);
    }

    PLOG_INFO << "hw_params rate set";

    if ((err = snd_pcm_hw_params_set_channels(m_capture_handle, m_hw_params, m_uNumChannels)) < 0)
    {
        PLOG_ERROR << "cannot set channel count (" << snd_strerror(err) << ")";
        exit(1);
    }

    PLOG_INFO << "hw_params channels set";

    if ((err = snd_pcm_hw_params(m_capture_handle, m_hw_params)) < 0)
    {
        PLOG_ERROR << "cannot set parameters (" << snd_strerror(err) << ")";
        exit(1);
    }

    PLOG_INFO << "hw_params set";

    snd_pcm_hw_params_free(m_hw_params);

    PLOG_INFO << "hw_params freed";

    if ((err = snd_pcm_prepare(m_capture_handle)) < 0)
    {
        PLOG_ERROR << "cannot prepare audio interface for use (" << snd_strerror(err) << ")";
        exit(1);
    }
}
