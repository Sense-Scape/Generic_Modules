#ifndef WAV_ACCUMULATOR
#define WAV_ACCUMULATOR

#include "BaseModule.h"
#include "WAVChunk.h"

class WAVAccumulator : public BaseModule
{

public:
    /**
     * @brief Construct a new Base Module object
     * @param[in] uAccumulatePeriod maximum number of seconds with which a module will make a recording
     * @param[in] uMaxInputBufferSize size of input buffer
     */
    WAVAccumulator(double dAccumulatePeriod, double dContinuityThresholdFactor, unsigned uMaxInputBufferSize);
    ~WAVAccumulator(){};

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    std::string GetModuleType() override { return "WAVAccumulatorModule"; };

private:
    unsigned m_dAccumulatePeriod;                                                       ///< Maximum number of seconds with which a module will make a recording
    double m_dContinuityThresholdFactor;                                                ///< Double which defines max jitter before signal considered discontinuous
    std::map<std::vector<uint8_t>, std::shared_ptr<BaseChunk>> m_mAccumulatedWAVChunks; ///< Map of accumulated WAV chunks as a function of string mac addr
    std::map<std::vector<uint8_t>, uint64_t> m_i64PreviousTimeStamps;                   ///<

    /*
     * @brief Module process to accumulate WAV chunks
     * @param[in] pBaseChunkpointer to WAVChunk
     */
    void AccumulateWAVChunk(std::shared_ptr<WAVChunk> pWAVChunk);

    /*
     * @brief Adds current chunk data to accumulated chunk
     * @param[in] pAccumulateWAVChunk pointer to accumulated WAV data
     * @param[in] pCurrentWAVChunk pointer to current WAV chunk
     */
    void Accumuluate(const std::shared_ptr<WAVChunk> &pAccumulatedWAVChunk,const std::shared_ptr<WAVChunk> &pCurrentWAVChunk);

    /*
     * @brief Verifies that current chunk is continuous with currently accumulated data
     * @param[in] pCurrentWAVChunk Pointer to current time chunk
     * @param[in] pAccumulatedWAVChunk  Pointer to accumulated time chunk
     */
    bool VerifyTimeContinuity(std::shared_ptr<WAVChunk> pCurrentWAVChunk, std::shared_ptr<WAVChunk> pAccumulatedWAVChunk);

    /*
     * @brief Checks if the current WAV chunk (recording) meets time threshold and can therefore be passed on
     * @param[in] pAccumulatedWAVChunk pointer to accumulated WAV chunk data
     */
    bool CheckMaxTimeThreshold(std::shared_ptr<WAVChunk> pAccumulatedWAVChunk);

    /*
     * @brief Checks if the end device recording mode has been changed since last accumulation
     * @param[in] pAccumulatedWAVChunk pAccumulatedWAVChunk pointer to accumulated WAV chunk data
     * @param[in] pCurrentWAVChunk pointer to current WAV chunk
     */
    bool WAVHeaderChanged(std::shared_ptr<WAVChunk> pAccumulatedWAVChunk, std::shared_ptr<WAVChunk> pCurrentWAVChunk);

protected:
    /*
     * @brief Module process to write WAV file
     * @param[in] pBaseChunkpointer to base chunk
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;
};

#endif
