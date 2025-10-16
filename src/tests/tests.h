///////////////////////////////////////////////////////////////////////////////
// tests.h
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TESTS__TESTS_H
#define MUD98__TESTS__TESTS_H

#include <lox/value.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef int (*TestFunc)();

typedef struct TestGroup TestGroup;

typedef struct TestCase {
    const char* name;
    TestFunc func;
    TestGroup* group;
} TestCase;

typedef struct TestGroup {
    const char* name;
    TestCase* tests;
    size_t count;
    size_t capacity;

    int asserts_failed;
} TestGroup;

void init_test_group(TestGroup* group, const char* name);

// All featured test groups
void register_lox_tests();
void register_lox_ext_tests();
void register_entity_tests(); 

void run_all_tests();

void test_fail(const char* expr, const char* file, int line);
#define ASSERT(expr) ((expr) ? (void)0 : test_fail(#expr, __FILE__, __LINE__))

void test_string_eq(const char* expected, const char* actual, const char* file, int line);
#define ASSERT_STR_EQ(expected, actual) test_string_eq(expected, actual, __FILE__, __LINE__)

void test_lox_int_eq(int expected, Value actual, const char* file, int line);
#define ASSERT_LOX_INT_EQ(expected, actual) test_lox_int_eq(expected, actual, __FILE__, __LINE__)

void test_lox_string_eq(const char* expected, Value actual, const char* file, int line);
#define ASSERT_LOX_STR_EQ(expected, actual) test_lox_string_eq(expected, actual, __FILE__, __LINE__)

#define ASSERT_OUTPUT_EQ(expected) \
    ASSERT(result == INTERPRET_OK); \
    ASSERT(IS_STRING(test_output_buffer)); \
    if (IS_STRING(test_output_buffer)) \
        ASSERT_STR_EQ(expected, AS_CSTRING(test_output_buffer)); 

extern bool test_dissasemble_on_error;
extern bool test_output_enabled;
extern Value test_output_buffer;

#endif // !MUD98__TESTS__TESTS_H
