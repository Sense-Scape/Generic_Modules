#ifndef SESSION_PROC_MODULE_TESTS
#define SESSION_PROC_MODULE_TESTS

#include "doctest.h"
#include "SessionProcModule.h"

TEST_CASE("SessionProcModule Test")
{
	unsigned uBufferSize = 10;
	SessionProcModule sessionProcModule(uBufferSize);

	CHECK(sessionProcModule.GetModuleType() == ModuleType::SessionProcModule);
}

#endif