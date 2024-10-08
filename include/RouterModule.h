#ifndef CHUNK_ROUTER
#define CHUNK_ROUTER

#include "BaseModule.h"
#include "ChunkDuplicatorUtility.h"

class RouterModule : public BaseModule
{
public:
    /**
     * @brief Construct a new Router Module forward data into different pipeline streams.
     * @param[in] uBufferSize size of processing input buffer
     */
    RouterModule(unsigned uBufferSize);
    ~RouterModule(){};

    /*
     * @brief Allows on to connect multiple modules to the ouput of this router
     *        Upon setting, will register the route - chunk type to pass to next module
     */
    void RegisterOutputModule(std::shared_ptr<BaseModule> pNextModule, ChunkType eChunkType);

    ///**
    // * @brief Returns module type
    // * @return ModuleType of processing module
    // */
    std::string GetModuleType() override { return "RouterModule"; };

protected:
    std::map<ChunkType, std::vector<std::shared_ptr<BaseModule>>> m_ChunkTypeModuleMap; ///< Shared pointer to next module into which messages are passed
    std::vector<ChunkType> m_vChunkTypesAlreadyLogged;                                  ///< Stores whether the user has been warned about the chunk not being registered

private:

    /*
     * @brief Passes chunk to relevant module
     * @param[in] pBaseChunk pointer to chunk that shall be routed
     */
    void RouteChunk(std::shared_ptr<BaseChunk> pBaseChunk);
};

#endif
