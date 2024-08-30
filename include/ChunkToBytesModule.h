#ifndef CHUNK_TO_BYTES_MODULE
#define CHUNK_TO_BYTES_MODULE

/*Standard Includes*/
#include <cmath>

/* Custom Includes */
#include "BaseModule.h"
#include "ByteChunk.h"
#include "SessionController.h"

/*
 * @brief Module process converts all chunks into multiple UDP chunks
 *   to pass onto transmission modules
 */
class ChunkToBytesModule : public BaseModule
{
public:
    /**
     * @brief Construct a new Router Module forward data into different pipeline streams.
     * @param[in] uBufferSize size of processing input buffer
     * @param[in] uTransmissionSize size of transmisison in bytes
     */
    ChunkToBytesModule(unsigned uBufferSize, unsigned uTransmissionSize);
    ~ChunkToBytesModule(){};

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    std::string GetModuleType() override { return "ChunkToBytesModule"; };

    /**
     * @brief Check input buffer and try process data
     */
    void ContinuouslyTryProcess() override;

private:
    unsigned m_uTransmissionSize;                                                                                                   ///< size of transmisison in bytes
    std::map<std::vector<uint8_t>, std::map<ChunkType, std::shared_ptr<SessionController>>> m_MapOfIndentifiersToChunkTypeSessions; ///< Map of each source identifier to specific chunk type being processed

    /*
     * @brief Module process to collect and format UDP data
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk);

};

#endif
