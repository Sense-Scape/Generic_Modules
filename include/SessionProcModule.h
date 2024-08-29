#ifndef SESSION_PROC_MODULE
#define SESSION_PROC_MODULE

/*Standard Includes*/

/* Custom Includes */
#include "BaseModule.h"
#include "SessionController.h"
#include "ByteChunk.h"
#include "TimeChunk.h"
#include "ChunkDuplicatorUtility.h"

class SessionProcModule : public BaseModule
{
public:
    /**
     * @brief Construct a new Session Processing Module to produce UDP data. responsible
     *  for acummulating all required UDP bytes and passing on for further processing
     * @param[in] uBufferSize size of processing input buffer
     */
    SessionProcModule(unsigned uBufferSize);
    ~SessionProcModule(){};

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    std::string GetModuleType() override { return "SessionProcModule"; };

private:
    std::map<uint32_t, std::function<void(std::shared_ptr<ByteChunk>)>> m_mFunctionCallbacksMap;             ///< Map of function callbacks called according to session type
    std::map<std::vector<uint8_t>, std::map<ChunkType, std::shared_ptr<std::vector<char>>>> m_mSessionBytes; ///< Map of session mode intermediate bytes prior ro session completion
    std::map<std::vector<uint8_t>, std::map<ChunkType, std::shared_ptr<SessionController>>> m_mSessionModesStatesMap;
    /*
     * @brief Module process to collect and format UDP data
     */
    void Process_ByteChunk(std::shared_ptr<BaseChunk> pBaseChunk);

    /*
     * @brief Registers all used session types for use
     */
    void RegisterSessionStates();

    std::shared_ptr<SessionController> GetPreviousSessionState(std::vector<uint8_t> &vu8SourceIdentifier, ChunkType chunkType);

    void UpdatePreviousSessionState(std::vector<uint8_t> &vu8SourceIdentifier, ChunkType chunkType, SessionController reliableSessionMode);
};

#endif