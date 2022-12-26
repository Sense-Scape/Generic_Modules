#ifndef WAV_ACCUMULATOR
#define WAV_ACCUMULATOR

#include "BaseModule.h"

class WAVAccumulator :
    public BaseModule
{
private:
    unsigned m_dAccumulatePeriod;                                               ///< Maximum number of seconds with which a module will make a recording
    std::map<std::string, std::shared_ptr<BaseChunk>> m_mAccumulatedWAVChunks;  ///< Map of accumulated WAV chunks as a function of string mac addr
    std::map<std::string, unsigned> m_mPreviousTimestamp;                       ///< Map of accumulated WAV chunk previous time stamps as a function of string mac addr

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
    void Accumuluate(std::shared_ptr<WAVChunk> pAccumulatedWAVChunk, std::shared_ptr<WAVChunk> pCurrentWAVChunk);

    /*
    * @brief Verifies that current chunk is continuous with currently accumulated data
    * @param[in] pAccumulatedWAVChunk pAccumulatedWAVChunk pointer to accumulated WAV chunk data
    * @param[in] pCurrentWAVChunk pointer to current WAV chunk
    */
    bool VerifyTimeContinuity(std::shared_ptr<WAVChunk> pAccumulatedWAVChunk, std::shared_ptr<WAVChunk> pCurrentWAVChunk);

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

public:
    /**
     * @brief Construct a new Base Module object
     * @param[in] uAccumulatePeriod maximum number of seconds with which a module will make a recording
     * @param[in] uMaxInputBufferSize size of input buffer
     */
    WAVAccumulator(double dAccumulatePeriod, unsigned uMaxInputBufferSize);
    ~WAVAccumulator() {};

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::WAVAccumulatorModule; };
};

#endif
