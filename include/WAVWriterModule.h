#ifndef WAV_WRITER_MODULE
#define WAV_WRITER_MODULE

/*Standard Includes*/
#include <time.h>
#include <iostream>
#include <filesystem>
#include <fstream>

/*Custom Includes*/
#include "BaseModule.h"
#include "WAVChunk.h"

/**
 * @brief Checks for space and writes WAV data to file system
 */
class WAVWriterModule : public BaseModule
{

private:
    std::string m_sFileWritePath; ///< String as to where WAV file are written

    /*
     * @brief Writes a WAV file to system
     */
    void WriteWAVFile(std::shared_ptr<BaseChunk> pBaseChunk);

    /*
     * @brief Creates file path if it does not exist
     */
    void CreateFilePath();

    /*
     * @brief Will check if there is enough space (100Mb) to write file
     * @return boolean whether there is enough space or not
     */
    bool IsEnoughFileSystemSpace();

protected:
    /*
     * @brief Module process to write WAV file
     */
    void Process_WAVChunk(std::shared_ptr<BaseChunk> pBaseChunk);

public:
    /*
     * Constructor
     * @param[in] sFileWritePath path into which WAV files will be written
     * @param[in] uMaxInputBufferSize size of input buffer
     */
    WAVWriterModule(std::string sFileWritePath, unsigned uMaxInputBufferSize);
    ~WAVWriterModule(){};

    /*
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    std::string GetModuleType() override { return "WAVWriterModule"; };
};

#endif