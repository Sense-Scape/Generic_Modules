#include "FFTModule.h"

FFTModule::FFTModule()
{

}

void FFTModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	TryPassChunk(pBaseChunk);
}