#include "TimeToWAVModule.h"

TimeToWAVModule::TimeToWAVModule(unsigned uBufferSize) : 
	BaseModule(uBufferSize)
{

}

void TimeToWAVModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	// Creating/casting shared pointers to data
	auto pTimeChunk = std::dynamic_pointer_cast<TimeChunk>(pBaseChunk);
	auto pWAVChunk = std::make_shared<WAVChunk>();

	// Convert time chunk to WAV
	ConvertTimeToWAV(pTimeChunk, pWAVChunk);

	// Try pass on
	TryPassChunk(pWAVChunk);
}

void TimeToWAVModule::ConvertTimeToWAV(std::shared_ptr<TimeChunk> pTimeChunk, std::shared_ptr<WAVChunk> pWAVChunk)
{
    // Setting timestamp
    pWAVChunk->m_i64TimeStamp = pTimeChunk->m_i64TimeStamp;

    // Creating WAV header
    pWAVChunk->m_sWAVHeader.SamplesPerSec = pTimeChunk->m_dSampleRate;
    pWAVChunk->m_sWAVHeader.NumOfChan = pTimeChunk->m_uNumChannels;
    pWAVChunk->m_sWAVHeader.bitsPerSample = pTimeChunk->m_uNumBytes * 8;
    pWAVChunk->m_sWAVHeader.SamplesPerSec = pTimeChunk->m_dSampleRate;
    pWAVChunk->m_sWAVHeader.blockAlign = pTimeChunk->m_uNumBytes*pTimeChunk->m_uNumChannels;
    pWAVChunk->m_sWAVHeader.bytesPerSec = pTimeChunk->m_dSampleRate * pWAVChunk->m_sWAVHeader.blockAlign;

    // Setting chunk Sizes
    pWAVChunk->m_sWAVHeader.Subchunk2Size = pTimeChunk->m_uNumChannels * pTimeChunk->m_dChunkSize * pTimeChunk->m_uNumBytes;
    pWAVChunk->m_sWAVHeader.ChunkSize = pWAVChunk->m_sWAVHeader.Subchunk2Size + 44 - 8; 

    // Now lets store the time data
    for (unsigned uSampleIndex = 0; uSampleIndex < pTimeChunk->m_dChunkSize; uSampleIndex++)
    {
        // Iterating through each audio channel
        for (auto vADCChannelData = pTimeChunk->m_vvi16TimeChunks.begin(); vADCChannelData != pTimeChunk->m_vvi16TimeChunks.end(); ++vADCChannelData)
            pWAVChunk->m_vi16Data.push_back((*vADCChannelData)[uSampleIndex]);

    }

}