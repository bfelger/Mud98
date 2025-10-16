///////////////////////////////////////////////////////////////////////////////
// test_registry.h
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TESTS__TEST_REGISTRY_H
#define MUD98__TESTS__TEST_REGISTRY_H

#include <lox/value.h>

#include <stdbool.h>

typedef int (*TestFunc)();

typedef struct TestCase {
    const char* name;
    TestFunc func;
} TestCase;

void register_test(const char* name, TestFunc func);
void run_all_tests();
void test_fail(const char* expr, const char* file, int line);
#define ASSERT(expr) ((expr) ? (void)0 : test_fail(#expr, __FILE__, __LINE__))

void test_string_eq(const char* expected, const char* actual, const char* file, int line);
#define ASSERT_STR_EQ(expected, actual) test_string_eq(expected, actual, __FILE__, __LINE__)

void test_lox_int_eq(int expected, Value actual, const char* file, int line);
#define ASSERT_LOX_INT_EQ(expected, actual) test_lox_int_eq(expected, actual, __FILE__, __LINE__)

void test_lox_string_eq(const char* expected, Value actual, const char* file, int line);
#define ASSERT_LOX_STR_EQ(expected, actual) test_lox_string_eq(expected, actual, __FILE__, __LINE__)

extern bool test_dissasemble_on_error;
extern bool test_output_enabled;

#endif // !MUD98__TESTS__TEST_REGISTRY_H
