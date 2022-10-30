#ifndef WAV_WRITER_MODULE
#define WAV_WRITER_MODULE

/*Standard Includes*/
#include <time.h>
#include <iostream>

/*Custom Includes*/
#include "BaseModule.h"

class WAVWriterModule :
    public BaseModule
{

private:
    std::string m_sFileWritePath;   ///< String as to where WAV file are written

    /*
    * @brief Writes a WAV file to system
    */
    void WriteWAVFile(std::shared_ptr<BaseChunk> pBaseChunk);

protected:
    /*
    * @brief Module process to write WAV file
    */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

public:

    /*
    * Constructor
    * @param[in] sFileWritePath path into which WAV files will be written
    * @param[in] uMaxInputBufferSize size of input buffer
    */
    WAVWriterModule(std::string sFileWritePath, unsigned uMaxInputBufferSize);
    ~WAVWriterModule() {};

    /*
    * @brief Returns module type
    * @param[out] ModuleType of processing module
    */
    ModuleType GetModuleType() override { return ModuleType::WAVWriterModule; };
};

#endif