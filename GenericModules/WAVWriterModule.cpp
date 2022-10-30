#include "WAVWriterModule.h"

WAVWriterModule::WAVWriterModule(std::string sFileWritePath, unsigned uMaxInputBufferSize) : BaseModule(uMaxInputBufferSize),
																							m_sFileWritePath(sFileWritePath)
{

}

void WAVWriterModule::WriteWAVFile(std::shared_ptr<BaseChunk> pBaseChunk)
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
	FILE *WAVRecordingFile;
	fopen_s(&WAVRecordingFile, sFileName.c_str(), "wb");

	// Writing WAV header
	fwrite(&pvcWAVHeaderBytes->at(0), sizeof(char), (*pvcWAVHeaderBytes).size(), WAVRecordingFile);
	
	// Writing data
	fwrite(&(pWAVChunk->m_vfData[0]), pWAVChunk->m_sWAVHeader.bitsPerSample / 8, pWAVChunk->m_vfData.size(), WAVRecordingFile);

	// Closing file
	fclose(WAVRecordingFile);

}

void WAVWriterModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	WriteWAVFile(pBaseChunk);
}
