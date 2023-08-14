#ifndef CHUNK_TO_BYTES_MODULE
#define CHUNK_TO_BYTES_MODULE

#include "BaseModule.h"
#include "ChunkTypesNamingUtility.h"
#include "UDPChunk.h"

#include <cmath>

/*
* @brief Module process converts all chunks into multiple UDP chunks
*   to pass onto transmission modules
*/
class ChunkToBytesModule :
    public BaseModule
{
public:
    /**
     * @brief Construct a new Router Module forward data into different pipeline streams.
     * @param[in] uBufferSize size of processing input buffer
     * @param[in] uTransmissionSize size of transmisison in bytes
     */
    ChunkToBytesModule(unsigned uBufferSize, unsigned uTransmissionSize);
    ~ChunkToBytesModule() {};

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::ChunkToBytesModule; };

private:
    unsigned m_uTransmissionSize;   ///< size of transmisison in bytes

    /*
     * @brief Module process to collect and format UDP data
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;
};

#endif
