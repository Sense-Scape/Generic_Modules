#ifndef ROUTER_MODULE_TESTS
#define ROUTER_MODULE_TESTS

#include "doctest.h"
#include "RouterModule.h"

TEST_CASE("BaseModule Test")
{
    unsigned uBufferSize = 10;
    RouterModule routerModule(uBufferSize);

    CHECK(routerModule.GetModuleType() == ModuleType::RouterModule);
}

#endif