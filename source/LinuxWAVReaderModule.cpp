#include "LinuxWAVReaderModule.h"

LinuxWAVReaderModule::LinuxWAVReaderModule(std::string sFileReadPath, uint32_t u32ChunkSize, unsigned uMaxInputBufferSize) : BaseModule(uMaxInputBufferSize),
                                                                                                                             m_sFileReadPath(sFileReadPath),
                                                                                                                             m_u16FilePlaybackIndex(0),
                                                                                                                             m_u32ChunkSize(u32ChunkSize),
                                                                                                                             m_vsFileList(0)
{
    SetInputWAVFileList();
}

void LinuxWAVReaderModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{

    // Check if currently reading and if not open a file
    if (!m_CurrentWAVFile)
    {
        m_CurrentWAVFile = sf_open(m_vsFileList[m_u16FilePlaybackIndex].c_str(), SFM_READ, &m_sfinfo);
        m_u16FilePlaybackIndex += 1;
        m_CurrentReadPosition = 0;

        auto strInfo = "Playback starting: " + m_vsFileList[0];
        PLOG_INFO << strInfo;
    }

    // Othewise carry on reading
    long lBufferLength = m_u32ChunkSize * m_sfinfo.channels;
    std::vector<int16_t> i16DataBuffer(lBufferLength); // Create a vector of int16_t
    sf_count_t frames_read = sf_read_short(m_CurrentWAVFile, i16DataBuffer.data(), lBufferLength);
    m_CurrentReadPosition += frames_read;

    // Now check if we are done the file and if so close
    if (m_CurrentReadPosition >= m_sfinfo.frames)
    {
        sf_close(m_CurrentWAVFile);
        m_CurrentWAVFile = nullptr;

        auto strInfo = "Playback finished: " + m_vsFileList[0];
        PLOG_INFO << strInfo;

        // Check if we have finished playing back all the files in the current directory and reset
        if (m_u16FilePlaybackIndex >= m_vsFileList.size())
            m_u16FilePlaybackIndex = 0;
    }

    // Once read convert to time chunk
    auto pTimeChunk = std::make_shared<TimeChunk>(m_u32ChunkSize, m_sfinfo.samplerate, 0, sizeof(int16_t), sizeof(int16_t) / 8, m_sfinfo.channels);
    for (uint16_t u16ChunkIndex = 0; u16ChunkIndex < m_u32ChunkSize; u16ChunkIndex++)
    {
        for (uint16_t u16ChannelIndex = 0; u16ChannelIndex < m_sfinfo.channels; u16ChannelIndex++)
            pTimeChunk->m_vvi16TimeChunks[u16ChannelIndex][u16ChunkIndex] = i16DataBuffer[u16ChunkIndex * u16ChannelIndex + u16ChunkIndex];
    }
    pTimeChunk->SetSourceIdentifier({0, 0});

    // Pass it on
    if (!TryPassChunk(std::static_pointer_cast<BaseChunk>(pTimeChunk)))
    {
        std::string strWarning = std::string(__FUNCTION__) + ": Next buffer full, dropping current chunk and passing \n";
        PLOG_WARNING << strWarning;
    }

    // And then sleep for the amount of time that was sent in chunk
    auto fChunkDuration_seconds = (float)m_u32ChunkSize / (float)m_sfinfo.samplerate;
    std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)(1000 * fChunkDuration_seconds)));
}

void LinuxWAVReaderModule::SetInputWAVFileList()
{
    // Find all files using dorectory iterator
    for (const auto &entry : std::filesystem::directory_iterator(m_sFileReadPath))
    {
        // The we check if it is wav and store it if it is
        if (entry.is_regular_file() && entry.path().extension() == ".wav")
            m_vsFileList.emplace_back(entry.path().string());
    }

    // If no file found then throw
    if (!m_vsFileList.size())
    {
        auto strError = "No WAV files found in: " + m_sFileReadPath;
        PLOG_INFO << strError;
        throw;
    }

    // Using a range-based for loop
    for (const std::string &sFilePath : m_vsFileList)
        PLOG_INFO << "LinuxWAVReaderModule found: " + sFilePath;
}

void LinuxWAVReaderModule::ContinuouslyTryProcess()
{
    while (!m_bShutDown)
    {
        auto pBaseChunk = std::make_shared<BaseChunk>();
        Process(pBaseChunk);
    }
}