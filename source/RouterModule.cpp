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
                std::string strWarning = std::string(__FUNCTION__) + ": " + pNextModule->GetModuleType() + " already registered, replacing \n";
                PLOG_WARNING << strWarning;

                vpNextModules[uNextModuleIndex] = pNextModule;
                return;
            }
        }
    }

    // If not, register the chunktype module pair
    m_ChunkTypeModuleMap[eChunkType].push_back(pNextModule);
    RegisterChunkCallbackFunction(eChunkType, &RouterModule::RouteChunk,(BaseModule*)this);
}

void RouterModule::RouteChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{   
    auto eCurrentChunkType = pBaseChunk->GetChunkType();

    bool bChunkRegistered = m_ChunkTypeModuleMap.count(eCurrentChunkType);
    bool bNonRegisteredChunkNotLogged = std::find(m_vChunkTypesAlreadyLogged.begin(), m_vChunkTypesAlreadyLogged.end(), eCurrentChunkType) == m_vChunkTypesAlreadyLogged.end();

    if (bChunkRegistered)
    {
        auto vpNextModules = m_ChunkTypeModuleMap[pBaseChunk->GetChunkType()];
        for (unsigned uNextModuleIndex = 0; uNextModuleIndex < vpNextModules.size(); uNextModuleIndex++)
        {
            // Copying data and passing down multiple pipeline connections
            // Duplicating to prevent two modules trying to access data
            auto pDuplicateBaseChunk = ChunkDuplicatorUtility::DuplicateDerivedChunk(pBaseChunk);
            vpNextModules[uNextModuleIndex]->TakeChunkFromModule(pDuplicateBaseChunk);
        }
    }

    if (!bChunkRegistered && bNonRegisteredChunkNotLogged)
    {
        // Then if the chunk has not been logged yet as not registered, log it
        m_vChunkTypesAlreadyLogged.emplace_back(eCurrentChunkType);
        std::string strWarning = std::string(__FUNCTION__) + ": " + ChunkTypesNamingUtility::toString(pBaseChunk->GetChunkType()) + " not registered chunk type, dropping";
        PLOG_WARNING << strWarning;
    }
}