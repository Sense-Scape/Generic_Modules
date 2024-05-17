#include "LinuxWAVWriterModule.h"

LinuxWAVWriterModule::LinuxWAVWriterModule(std::string sFileWritePath, unsigned uMaxInputBufferSize) : BaseModule(uMaxInputBufferSize),
                                                                                                       m_sFileWritePath(sFileWritePath)
{
    // Creating file path for audio files if it does not exist
    CreateFilePath();
}

void LinuxWAVWriterModule::WriteWAVFile(std::shared_ptr<BaseChunk> pBaseChunk)
{
    auto pWAVChunk = std::dynamic_pointer_cast<WAVChunk>(pBaseChunk);

    std::cout << pWAVChunk->GetHeaderString() << std::endl;

    // Creating file name - fs_NumChannels_Epoch
    time_t ltime;
    time(&ltime);
    std::string sFileName = m_sFileWritePath + "/Audio_fs_" + std::to_string(pWAVChunk->m_sWAVHeader.SamplesPerSec) + "_Chans_" + std::to_string(pWAVChunk->m_sWAVHeader.NumOfChan) + "_" + std::to_string((long long)ltime) + ".wav";

    // These will all by default be little endian
    auto pvcWAVHeaderBytes = pWAVChunk->WAVHeaderToBytes();

    // Creating and opening file
    FILE *WAVRecordingFile = fopen(sFileName.c_str(), "wb");

    // Writing WAV header
    fwrite(&pvcWAVHeaderBytes->at(0), sizeof(char), (*pvcWAVHeaderBytes).size(), WAVRecordingFile);

    // Writing data - data - bytes - size of
    fwrite(&(pWAVChunk->m_vi16Data[0]), pWAVChunk->m_sWAVHeader.bitsPerSample / 8, pWAVChunk->m_vi16Data.size(), WAVRecordingFile);

    // Closing file
    fclose(WAVRecordingFile);
}

void LinuxWAVWriterModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    WriteWAVFile(pBaseChunk);
}

void LinuxWAVWriterModule::CreateFilePath()
{
    if (!std::filesystem::exists(m_sFileWritePath))
    {
        std::filesystem::create_directory(m_sFileWritePath);
        std::string strInfo = std::string(__FUNCTION__) + ": Folder created successfully - " + m_sFileWritePath;
        PLOG_INFO << strInfo;
    }
}