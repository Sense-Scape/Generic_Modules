#ifndef CHUNKROUTER
#define CHUNKROUTER

#include "BaseModule.h"

class RouterModule : 
    public BaseModule
{
protected:                 
    std::map<ChunkType, std::vector<std::shared_ptr<BaseModule>>> m_ChunkTypeModuleMap;        ///< Shared pointer to next module into which messages are passed

private:

    /*
     * @brief Module process to collect and format UDP data
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /*
    * @brief Passes chunk to relevant module
    */
    void RouteChunk(std::shared_ptr<BaseChunk> pBaseChunk);

public:

    /**
     * @brief Construct a new Router Module forward data into different pipeline streams. 
     *
     * @param uBufferSize size of processing input buffer
     */
    RouterModule(unsigned uBufferSize);
    ~RouterModule() {};

    /*
     * @brief Allows on to connect multiple modules to the ouput of this router
     *        Upon setting, will register the route - chunk type to pass to next module
     */
    void RegisterOutputModule(std::shared_ptr<BaseModule> pNextModule, ChunkType eChunkType);

    ///**
    // * @brief Returns module type
    // * @param[out] ModuleType of processing module
    // */
    ModuleType GetModuleType() override { return ModuleType::RouterModule; };

};

#endif
