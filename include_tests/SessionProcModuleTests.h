#ifndef SESSION_PROC_MODULE_TESTS
#define SESSION_PROC_MODULE_TESTS

#include "doctest.h"
#include "SessionProcModule.h"

TEST_CASE("SessionProcModule Test")
{
	// Lets start by creating the session proc module
	unsigned uBufferSize = 10;
	SessionProcModule SessionProcModuleTestClass(uBufferSize);
	SessionProcModuleTestClass.SetTestMode(true);

	CHECK(SessionProcModuleTestClass.GetModuleType() == ModuleType::SessionProcModule);

	// Create a JSON Chunk
	auto pJSONChunk = std::make_shared<JSONChunk>();
	pJSONChunk->m_JSONDocument["test"] = "confirmed";
	auto pcJSONBytes = pJSONChunk->Serialise();

	// Emulate a transmission using the session mode class

	// Check if received class pointer is the same
}

#endif