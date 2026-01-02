#ifndef WAV_WRITER_MODULE
#define WAV_WRITER_MODULE

#include "BaseModule.h"

/**
 * @brief Checks for space and writes WAV data to file system
 */
class WAVWriterModule : public BaseModule {

public:
  /*
   * Constructor
   * @param[in] uMaxInputBufferSize size of input buffer
   * @param[in] jsonConfig JSON configuration object
   */
  WAVWriterModule(unsigned uMaxInputBufferSize,
                  nlohmann::json_abi_v3_11_2::json jsonConfig);
  ~WAVWriterModule() {};

  /*
   * @brief Returns module type
   * @param[out] ModuleType of processing module
   */
  std::string GetModuleType() override { return "WAVWriterModule"; };

private:
  const std::string
      m_sFileWritePath; ///< String as to where WAV file are written

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
};

#endif
