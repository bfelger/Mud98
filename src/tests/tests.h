///////////////////////////////////////////////////////////////////////////////
// tests.h
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TESTS__TESTS_H
#define MUD98__TESTS__TESTS_H

#include <lox/object.h>
#include <lox/value.h>
#include <lox/vm.h>

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
void register_container_tests();
void register_act_tests();
void register_fmt_tests();
void register_util_tests(); 
void register_event_tests();
void register_faction_tests();
void register_money_tests();

void run_unit_tests();

void test_fail(const char* expr, const char* file, int line);
#define ASSERT(expr) ((expr) ? (void)0 : test_fail(#expr, __FILE__, __LINE__))
#define ASSERT_OR_GOTO(expr, or_goto) if (!(expr)) {  \
    test_fail(#expr, __FILE__, __LINE__); \
    goto or_goto; }

void test_string_eq(const char* expected, const char* actual, const char* file, int line);
#define ASSERT_STR_EQ(expected, actual) test_string_eq(expected, actual, __FILE__, __LINE__)

void test_string_match(const char* pattern, const char* actual, const char* file, int line);
#define ASSERT_MATCH(pattern, actual) test_string_match(pattern, actual, __FILE__, __LINE__)

void test_lox_int_eq(int expected, Value actual, const char* file, int line);
#define ASSERT_LOX_INT_EQ(expected, actual) test_lox_int_eq(expected, actual, __FILE__, __LINE__)

void test_lox_string_eq(const char* expected, Value actual, const char* file, int line);
#define ASSERT_LOX_STR_EQ(expected, actual) test_lox_string_eq(expected, actual, __FILE__, __LINE__)

void test_lox_string_contains(const char* substring, Value actual, const char* file, int line);
#define ASSERT_LOX_STR_CONTAINS(substring, actual) test_lox_string_contains(substring, actual, __FILE__, __LINE__)

#define ASSERT_OUTPUT_EQ(expected) \
    ASSERT(IS_STRING(test_output_buffer)); \
    if (IS_STRING(test_output_buffer) && AS_STRING(test_output_buffer) != NULL) \
        ASSERT_STR_EQ(expected, AS_CSTRING(test_output_buffer))

#define ASSERT_OUTPUT_MATCH(pattern) \
    ASSERT(IS_STRING(test_output_buffer)); \
    if (IS_STRING(test_output_buffer) && AS_STRING(test_output_buffer) != NULL) \
        ASSERT_MATCH(pattern, AS_CSTRING(test_output_buffer))

#define ASSERT_OUTPUT_CONTAINS(substring) \
    ASSERT(IS_STRING(test_output_buffer)); \
    if (IS_STRING(test_output_buffer) && AS_STRING(test_output_buffer) != NULL) \
        ASSERT_LOX_STR_CONTAINS(substring, test_output_buffer); 

#define ASSERT_LOX_OUTPUT_EQ(expected) \
    ASSERT(result == INTERPRET_OK); \
    ASSERT_OUTPUT_EQ(expected) 

extern bool test_trace_exec;
extern bool test_disassemble_on_error;
extern bool test_output_enabled;
extern bool test_act_output_enabled;
extern bool test_socket_output_enabled;
extern Value test_output_buffer;

#endif // !MUD98__TESTS__TESTS_H
