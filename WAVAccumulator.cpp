#include "WAVAccumulator.h"

WAVAccumulator::WAVAccumulator(double dAccumulatePeriod, unsigned uMaxInputBufferSize) : BaseModule(uMaxInputBufferSize),
																							m_dAccumulatePeriod(dAccumulatePeriod),
																							m_mAccumulatedWAVChunks(),
																							m_mPreviousTimestamp()
{

}

void WAVAccumulator::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	auto pWAVChunk = std::static_pointer_cast<WAVChunk>(pBaseChunk);
	AccumulateWAVChunk(pWAVChunk);
}

void WAVAccumulator::Accumuluate(std::shared_ptr<WAVChunk> pAccumulatedWAVChunk, std::shared_ptr<WAVChunk> pCurrentWAVChunk)
{
	// Accumulate time data
	for (unsigned uDataIndex = 0; uDataIndex < pCurrentWAVChunk->m_vfData.size(); uDataIndex++)
		pAccumulatedWAVChunk->m_vfData.emplace_back(pCurrentWAVChunk->m_vfData[uDataIndex]);

	// Adjust WAV header
	pAccumulatedWAVChunk->m_sWAVHeader.ChunkSize += (pCurrentWAVChunk->m_sWAVHeader.ChunkSize + 8 - 44); // + 8 to whole file, -44 to remove header
	pAccumulatedWAVChunk->m_sWAVHeader.Subchunk2Size += pCurrentWAVChunk->m_vfData.size() * pCurrentWAVChunk->m_sWAVHeader.bitsPerSample;
}

bool WAVAccumulator::VerifyTimeContinuity(std::shared_ptr<WAVChunk> pAccumulateDWAVChunk, std::shared_ptr<WAVChunk> pCurrentWAVChunk)
{
	// TODO: Implement time checks
	return true;
}

bool WAVAccumulator::CheckMaxTimeThreshold(std::shared_ptr<WAVChunk> pAccumulateDWAVChunk)
{
	double dAccumulatedPeriod = ((double)pAccumulateDWAVChunk->m_vfData.size() / (double)pAccumulateDWAVChunk->m_sWAVHeader.NumOfChan) * (1/(double)pAccumulateDWAVChunk->m_sWAVHeader.SamplesPerSec);
	if (dAccumulatedPeriod > m_dAccumulatePeriod)
	{
		std::cout << std::string(__FUNCTION__) + ": WAV recoding of " + std::to_string(dAccumulatedPeriod) + " seconds made \n";
		return true;
	}
	else
		return false;
}

bool WAVAccumulator::WAVHeaderChanged(std::shared_ptr<WAVChunk> pAccumulateDWAVChunk, std::shared_ptr<WAVChunk> pCurrentWAVChunk)
{
	// TODO: Implement
	return false;
}

void WAVAccumulator::AccumulateWAVChunk(std::shared_ptr<WAVChunk> pWAVChunk)
{
	// Check if current MAC is being accumulated and store
	if (m_mAccumulatedWAVChunks.count(pWAVChunk->m_sMACAddress) == 0)
	{
		m_mAccumulatedWAVChunks[pWAVChunk->m_sMACAddress] = pWAVChunk;
	}
	else
	{
		// Try accumulate data
		auto pAccumulatedWAVChunk = std::static_pointer_cast<WAVChunk>(m_mAccumulatedWAVChunks[pWAVChunk->m_sMACAddress]);
		if (VerifyTimeContinuity(pAccumulatedWAVChunk, pWAVChunk) || !WAVHeaderChanged(pAccumulatedWAVChunk, pWAVChunk))
			Accumuluate(pAccumulatedWAVChunk, pWAVChunk);

		// Check to pass data on
		if (CheckMaxTimeThreshold(pAccumulatedWAVChunk))
		{
			// Pass and clear
			TryPassChunk(pAccumulatedWAVChunk);
			m_mAccumulatedWAVChunks.erase(pWAVChunk->m_sMACAddress);
		}
	}
}