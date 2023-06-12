#ifndef SESSION_PROC_MODULE
#define SESSION_PROC_MODULE

/*Standard Includes*/

/* Custom Includes */
#include "BaseModule.h"
#include "UDPChunk.h"
#include "WAVChunk.h"
#include "SessionModeTypes.h"


class SessionProcModule :
    public BaseModule
{
public:
    /**
     * @brief Construct a new Session Processing Module to produce UDP data. responsible
     *  for acummulating all required UDP bytes and passing on for further processing
     * @param[in] uBufferSize size of processing input buffer
     */
    SessionProcModule(unsigned uBufferSize);
    ~SessionProcModule() {};

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::SessionProcModule; };


private:
    std::map<uint32_t, std::function<void(std::shared_ptr<UDPChunk>)>> m_mFunctionCallbacksMap; ///< Map of function callbacks called according to session type
    std::map<std::vector<uint8_t>, std::map<ChunkType, std::shared_ptr<std::vector<char>>>> m_mSessionBytes;             ///< Map of session mode intermediate bytes prior ro session completion
    std::map<std::vector<uint8_t>, std::map<ChunkType, std::shared_ptr<SessionModeBase>>> m_mSessionModesStatesMap;
    /*
     * @brief Module process to collect and format UDP data
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /*
    * @brief Registers all used session types for use
    */
    void RegisterSessionStates();

    std::shared_ptr<TimeChunkSessionMode> GetPreviousSessionState(std::shared_ptr<BaseChunk> pBaseChunk, ChunkType chunkType);
};

#endif