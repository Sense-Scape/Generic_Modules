#ifndef SESSION_PROC_MODULE
#define SESSION_PROC_MODULE

/*Standard Includes*/

/*Custom Includes*/
#include "BaseModule.h"
#include "UDPChunk.h"
#include "WAVChunk.h"
#include "SessionModeTypes.h"


class SessionProcModule :
    public BaseModule
{
private:
    std::map<SessionModeTypes, std::function<void(std::shared_ptr<UDPChunk>)>> m_mFunctionCallbacksMap; ///< Map of function callbacks called according to session type
    std::map<SessionModeTypes, std::shared_ptr<SessionModeBase>> m_mSessionModesStatesMap;              ///< Map of session modes to track session mode states
    std::map<SessionModeTypes, std::shared_ptr<std::vector<char>>> m_mSessionBytes;                                      ///< Map of session mode intermediate bytes prior ro session completion

    /*
     * @brief Module process to collect and format UDP data
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /*
    * @brief Registers all UDP Chunk function callbacks according session mode types
    */
    void RegisterFunctionHandlers();

    /*
    * @brief Registers all used session types for use
    */
    void RegisterSessionStates();

    /*
    * @brief WAV Session callback. Extracts data and converts to a wav recording.
    *        when recording complete passes on otherwise if it will clear all
    *        data in the case a UDP chunk is missed
    */
    void ProcessWAVSession(std::shared_ptr<UDPChunk> pUDPChunk);

public:
    /**
     * @brief Construct a new Session Processing Module to produce UDP data. responsible
     *  for acummulating all required UDP bytes and passing on for further processing
     * 
     * @param uBufferSize size of processing input buffer
     */
    SessionProcModule(unsigned uBufferSize);
    ~SessionProcModule() {};

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::SessionProcModule; };

};

#endif