#include "RouterModule.h"

RouterModule::RouterModule(unsigned uBufferSize) : BaseModule(uBufferSize),
                                                    m_ChunkTypeModuleMap()
{

}

void RouterModule::RegisterOutputModule(std::shared_ptr<BaseModule> pNextModule, ChunkType eChunkType)
{
    // Checking if chunk type and module pair already registered
    if (m_ChunkTypeModuleMap.count(eChunkType))
    {
        auto vpNextModules = m_ChunkTypeModuleMap[eChunkType];
        for (unsigned uNextModuleIndex = 0; uNextModuleIndex < vpNextModules.size(); uNextModuleIndex++)
        {
            if (vpNextModules[uNextModuleIndex]->GetModuleType() == pNextModule->GetModuleType())
            {
                std::cout << std::string(__FUNCTION__) + ": " + ModuleTypeStrings::toString(pNextModule->GetModuleType()) + " already registered, replacing \n";
                vpNextModules[uNextModuleIndex] = pNextModule;
                return;
            }
        }
    }
    
    // If not, register the chunktype module pair
    m_ChunkTypeModuleMap[eChunkType].push_back(pNextModule);
}

void RouterModule::RouteChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    if (m_ChunkTypeModuleMap.count(pBaseChunk->GetChunkType())) 
    {
        auto vpNextModules = m_ChunkTypeModuleMap[pBaseChunk->GetChunkType()];
        for (unsigned uNextModuleIndex = 0; uNextModuleIndex < vpNextModules.size(); uNextModuleIndex++)
        {
            // Copying data and passing down multiple pipeline connections
            // Duplicating to prevent two modules trying to access data
           auto pDuplicateBaseChunk  = ChunkDuplicatorUtility::DuplicateDerivedChunk(pBaseChunk);
           vpNextModules[uNextModuleIndex]->TakeChunkFromModule(pDuplicateBaseChunk);
        }     
    }
    else
        std::cout << std::string(__FUNCTION__) + ": " + ChunkTypesNamingUtility::toString(pBaseChunk->GetChunkType())  + " not registered chunk type, dropping \n";
}

void RouterModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    RouteChunk(pBaseChunk);
}
