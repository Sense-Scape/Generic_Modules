#include "WAVWriterModule.h"

WAVWriterModule::WAVWriterModule(std::string sFileWritePath, unsigned uMaxInputBufferSize) : BaseModule(uMaxInputBufferSize),
                                                                                                       m_sFileWritePath(sFileWritePath)
{
    // Creating file path for audio files if it does not exist
    CreateFilePath();

    RegisterChunkCallbackFunction(ChunkType::WAVChunk, &WAVWriterModule::Process_WAVChunk);
}

void WAVWriterModule::WriteWAVFile(std::shared_ptr<BaseChunk> pBaseChunk)
{
    auto pWAVChunk = std::dynamic_pointer_cast<WAVChunk>(pBaseChunk);

    std::cout << pWAVChunk->GetHeaderString() << std::endl;

    // Creating file name
    time_t ltime;
    time(&ltime);
    std::string sFileName = m_sFileWritePath + "/Audio_fs_" + 
        std::to_string(pWAVChunk->m_sWAVHeader.SamplesPerSec) + "_Chans_" + 
        std::to_string(pWAVChunk->m_sWAVHeader.NumOfChan) + "_" + 
        std::to_string((long long)ltime) + ".wav";

    // Open file
    std::ofstream outFile(sFileName, std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open file: " << sFileName << std::endl;
        return; // or handle error appropriately
    }

    // Modify WAV header for 32-bit float
    pWAVChunk->m_sWAVHeader.AudioFormat = 3; // 3 for IEEE float
    pWAVChunk->m_sWAVHeader.bitsPerSample = 32;
    pWAVChunk->m_sWAVHeader.bytesPerSec = pWAVChunk->m_sWAVHeader.SamplesPerSec * pWAVChunk->m_sWAVHeader.NumOfChan * 4; // 4 bytes per sample
    pWAVChunk->m_sWAVHeader.blockAlign = pWAVChunk->m_sWAVHeader.NumOfChan * 4;

    // Recalculate data size
    uint32_t dataSize = pWAVChunk->m_vi16Data.size() * sizeof(float);
    pWAVChunk->m_sWAVHeader.Subchunk2Size = dataSize;

    // Write modified WAV header
    auto pvcWAVHeaderBytes = pWAVChunk->WAVHeaderToBytes();
    outFile.write(reinterpret_cast<const char*>(pvcWAVHeaderBytes->data()), 44);

    // Convert 16-bit int to 32-bit float and write
    std::vector<float> floatData(pWAVChunk->m_vi16Data.size());
    for (size_t i = 0; i < pWAVChunk->m_vi16Data.size(); ++i) {
        floatData[i] = static_cast<float>(pWAVChunk->m_vi16Data[i]) / 32768.0f;   
    }

    outFile.write(reinterpret_cast<const char*>(floatData.data()), dataSize);
    
    // Update file size in header
    uint32_t fileSize = static_cast<uint32_t>(outFile.tellp()) - 8;
    outFile.seekp(4);
    outFile.write(reinterpret_cast<const char*>(&fileSize), 4);
}

void WAVWriterModule::Process_WAVChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    if (IsEnoughFileSystemSpace())
        WriteWAVFile(pBaseChunk);
    else
    {
        std::string strWarning = std::string(__FUNCTION__) + "Not enough space to write wav file, data lost";
        PLOG_WARNING << strWarning;
    }
}

void WAVWriterModule::CreateFilePath()
{
    if (!std::filesystem::exists(m_sFileWritePath))
    {
        std::filesystem::create_directory(m_sFileWritePath);
        std::string strInfo = std::string(__FUNCTION__) + ": Folder created successfully - " + m_sFileWritePath;
        PLOG_INFO << strInfo;
    }
}

bool WAVWriterModule::IsEnoughFileSystemSpace()
{
    std::filesystem::space_info space = std::filesystem::space(m_sFileWritePath);
    return space.available > 100'000'000;   
}