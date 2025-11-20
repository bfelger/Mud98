///////////////////////////////////////////////////////////////////////////////
// test_registry.h
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TESTS__TEST_REGISTRY_H
#define MUD98__TESTS__TEST_REGISTRY_H

#include "tests.h"

void register_test(TestGroup* group, const char* name, TestFunc func);
void register_test_group(TestGroup* group);

#endif // !MUD98__TESTS__TEST_REGISTRY_H
