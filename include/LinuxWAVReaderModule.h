#ifndef WAV_READER_MODULE
#define WAV_READER_MODULE

/*Standard Includes*/
#include <time.h>
#include <iostream>
#include <filesystem>
#include <vector>

/*Custom Includes*/
#include "BaseModule.h"
#include "TimeChunk.h"

/*3rd Party*/
#include <sndfile.hh> // For WAV Files

class LinuxWAVReaderModule : public BaseModule
{

private:
    std::string m_sFileReadPath;           ///< String from where files shall be read
    std::vector<std::string> m_vsFileList; ///< List of WAV files that shall be read
    uint16_t m_u16FilePlaybackIndex;       ///< Index of the file that should be played back
    uint32_t m_u32ChunkSize;               ///< Size of TimeChunk chunks
    SNDFILE *m_CurrentWAVFile;             ///< File that is currently being read
    SF_INFO m_sfinfo;                      ///< Storessou$d file meta data
    sf_count_t m_CurrentReadPosition;      ///< Stores position at which one is in file for playback

    /*
     * @brief updates the list of palyable files using specificed member direcotry
     */
    void SetInputWAVFileList();

protected:
    /*
     * @brief Module process to write WAV file
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

public:
    /*
     * Constructor
     * @param[in] sFileReadPath path from which WAV files shall be read
     * @param[in] u32ChunkSize size of TimeChunk chunks
     * @param[in] uMaxInputBufferSize size of input buffer
     */
    LinuxWAVReaderModule(std::string sFileReadPath, uint32_t u32ChunkSize, uint32_t uMaxInputBufferSize);
    ~LinuxWAVReaderModule(){};

    /*
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    std::string GetModuleType() override { return "LinuxWAVReaderModule"; };

    /**
     * @brief Check input buffer and try process data
     */
    void ContinuouslyTryProcess() override;
};

#endif