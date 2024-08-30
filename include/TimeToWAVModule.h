#ifndef TIME_TO_WAV_MODULE
#define TIME_TO_WAV_MODULE

/* Custom Includes */
#include "BaseModule.h"
#include "WAVChunk.h"
#include "TimeChunk.h"

/**
 * @brief Convert TimeChunks to WAVChunks
 */
class TimeToWAVModule : public BaseModule
{
public:
    /**
     * @brief Construct a new Session Processing Module to produce UDP data. responsible
     *  for acummulating all required UDP bytes and passing on for further processing
     * @param[in] uBufferSize size of processing input buffer
     */
    TimeToWAVModule(unsigned uBufferSize);
    ~TimeToWAVModule(){};

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    std::string GetModuleType() override { return "TimeToWavModule"; };

    /*
     * @brief Module process to write WAV file
     * @param[in] pBaseChunkpointer to base chunk
     */
    void Process_TimeChunk(std::shared_ptr<BaseChunk> pBaseChunk);

    /*
     * @brief Function that converts a time chunk into WAV chunk
     * @param[in] pTimeChunk shared pointer to time chunk
     * @param[in] pWAVChunk shared pointer to wav chunk
     */
    void ConvertTimeToWAV(std::shared_ptr<TimeChunk> pTimeChunk, std::shared_ptr<WAVChunk> pWAVChunk);
};

#endif
