#include "WAVAccumulator.h"

WAVAccumulator::WAVAccumulator(double dAccumulatePeriod, double dContinuityThresholdFactor, unsigned uMaxInputBufferSize)
	: BaseModule(uMaxInputBufferSize),
	m_dAccumulatePeriod(dAccumulatePeriod),
	m_dContinuityThresholdFactor(dContinuityThresholdFactor),
	m_mAccumulatedWAVChunks()
{
	RegisterChunkCallbackFunction(ChunkType::WAVChunk, &WAVAccumulator::Process_WAVChunk,(BaseModule*)this);
}

void WAVAccumulator::Process_WAVChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
	auto pWAVChunk = std::static_pointer_cast<WAVChunk>(pBaseChunk);
	AccumulateWAVChunk(pWAVChunk);
}

void WAVAccumulator::Accumuluate(const std::shared_ptr<WAVChunk> &pAccumulatedWAVChunk, const std::shared_ptr<WAVChunk> &pCurrentWAVChunk)
{
	// Accumulate time data
	for (unsigned uDataIndex = 0; uDataIndex < pCurrentWAVChunk->m_vi16Data.size(); uDataIndex++)
		pAccumulatedWAVChunk->m_vi16Data.push_back(pCurrentWAVChunk->m_vi16Data[uDataIndex]);

	// Adjust WAV header
	pAccumulatedWAVChunk->m_sWAVHeader.ChunkSize += (pCurrentWAVChunk->m_sWAVHeader.ChunkSize + 8 - 44); // + 8 to whole file, -44 to remove header
	pAccumulatedWAVChunk->m_sWAVHeader.Subchunk2Size += pCurrentWAVChunk->m_vi16Data.size() * pCurrentWAVChunk->m_sWAVHeader.bitsPerSample;
}

bool WAVAccumulator::VerifyTimeContinuity(std::shared_ptr<WAVChunk> pCurrentWAVChunk, std::shared_ptr<WAVChunk> pAccumulatedWAVChunk)
{
	// Lets start by gettinvg how much time has passed since this chunk was taken (delta = fs*samples)
	double dAccumulatedPeriod = (pAccumulatedWAVChunk->m_sWAVHeader.ChunkSize/ pAccumulatedWAVChunk->m_sWAVHeader.NumOfChan) * (1 / (double)pAccumulatedWAVChunk->m_sWAVHeader.SamplesPerSec);
	// lets now convert the accumulated period to microseconds
	uint64_t u64MircoAccumulatedPeriod = (uint64_t)(dAccumulatedPeriod * 1e6);
	// And then calcualte the expected time stamp
	uint64_t u64ExpectedCurrentTimeStamp = m_i64PreviousTimeStamps[pCurrentWAVChunk->GetSourceIdentifier()] +u64MircoAccumulatedPeriod;

	// Lets then see what the difference between the true and expected timestamp is
	uint64_t u64TimeDifferential = pCurrentWAVChunk->m_i64TimeStamp - u64ExpectedCurrentTimeStamp;
	// lets set the continuity threshold to 1 percent of the accumulation period
	uint64_t u64ContinuityThreshold = dAccumulatedPeriod * 1e6 * m_dContinuityThresholdFactor;
	bool bContinuous = false;

	// lets now determine if the stream is continuous
	int iTimeStampDifference = u64ExpectedCurrentTimeStamp - pCurrentWAVChunk->m_i64TimeStamp;
	if (std::abs(iTimeStampDifference) < u64ContinuityThreshold)
		bContinuous = true;

	// return bool whether within bound or not
	// Now that we have completed all the checks we can update the previous timestamp to current for future checks
	m_i64PreviousTimeStamps[pCurrentWAVChunk->GetSourceIdentifier()] = pCurrentWAVChunk->m_i64TimeStamp;

	if (!bContinuous)
	{
		std::string strWarning = std::string(__FUNCTION__) + ": Discontinuity found in WAV stream \n";
		PLOG_WARNING << strWarning;
	}

	return bContinuous;
}

bool WAVAccumulator::CheckMaxTimeThreshold(std::shared_ptr<WAVChunk> pAccumulateDWAVChunk)
{
	double dAccumulatedPeriod = ((double)pAccumulateDWAVChunk->m_vi16Data.size() / (double)pAccumulateDWAVChunk->m_sWAVHeader.NumOfChan) * (1/(double)pAccumulateDWAVChunk->m_sWAVHeader.SamplesPerSec);

	if (dAccumulatedPeriod > m_dAccumulatePeriod)
	{
		std::string strInfo = std::string(__FUNCTION__) + ": WAV recoding of " + std::to_string(dAccumulatedPeriod) + " seconds made \n";
		PLOG_INFO << strInfo;
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

	auto vu8SourceIdentifier = pWAVChunk->GetSourceIdentifier();

	// Check if current MAC is being accumulated and store
	if (m_mAccumulatedWAVChunks[vu8SourceIdentifier] == nullptr)
	{
		m_mAccumulatedWAVChunks[vu8SourceIdentifier] = pWAVChunk;
		m_i64PreviousTimeStamps[vu8SourceIdentifier] = pWAVChunk->m_i64TimeStamp;
		return;
	}

	// Try accumulate data if data is continuous and unchanged
	auto pAccumulatedWAVChunk = std::static_pointer_cast<WAVChunk>(m_mAccumulatedWAVChunks[vu8SourceIdentifier]);
	bool bChunkContinuous = VerifyTimeContinuity(pWAVChunk, pAccumulatedWAVChunk);
	bool bWAVHeaderChanged = !WAVHeaderChanged(pAccumulatedWAVChunk, pWAVChunk);

	if(bChunkContinuous || bWAVHeaderChanged)
		Accumuluate(pAccumulatedWAVChunk, pWAVChunk);
	else
	{
		std::string strWarning = std::string(__FUNCTION__) + ": Passing WAV data on early \n";
		PLOG_WARNING << strWarning;

		TryPassChunk(std::move(pAccumulatedWAVChunk));
		m_mAccumulatedWAVChunks[vu8SourceIdentifier] = nullptr;
		return;
	}
	
	// Check to pass data on
	bool bPassData = CheckMaxTimeThreshold(pAccumulatedWAVChunk);
	if (bPassData)
	{
		TryPassChunk(std::move(pAccumulatedWAVChunk));
		m_mAccumulatedWAVChunks[vu8SourceIdentifier] = nullptr;
		return;
	}
}