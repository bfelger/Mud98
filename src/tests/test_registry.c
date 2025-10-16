///////////////////////////////////////////////////////////////////////////////
// test_registry.c
///////////////////////////////////////////////////////////////////////////////

#include "test_registry.h"

#include <lox/value.h>
#include <lox/object.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static TestCase* tests = NULL;
static size_t test_count = 0;
static size_t test_capacity = 0;

static int tests_run = 0;
static int asserts_failed = 0;

extern bool test_output_enabled;

void register_test(const char* name, TestFunc func)
{
    if (test_count >= test_capacity) {
        size_t new_capacity = test_capacity == 0 ? 8 : test_capacity * 2;
        TestCase* new_tests = realloc(tests, new_capacity * sizeof(TestCase));
        if (!new_tests) {
            fprintf(stderr, "Failed to allocate memory for test registry.\n");
            exit(1);
        }
        tests = new_tests;
        test_capacity = new_capacity;
    }
    tests[test_count].name = name;
    tests[test_count].func = func;
    test_count++;
}

void run_all_tests()
{
    int failures = 0;
    printf("\nRunning %zu tests:\n\n", test_count);
    for (size_t i = 0; i < test_count; i++) {
        printf("Running test: %-40s", tests[i].name);
        asserts_failed = 0;
        
        tests[i].func();
        if (asserts_failed == 0) {
            printf("[\033[92mPASSED\033[0m]\n");
        } else {
            failures++;
        }
    }
    printf("\n%d out of %d tests failed.\n\n", (int)failures, (int)test_count);
    if (failures > 0) {
        exit(1);
    }
}

void test_fail(const char* expr, const char* file, int line)
{
    if (asserts_failed == 0)
        printf("[\033[91mFAILED\033[0m]\n");
    asserts_failed++;
    fprintf(stderr, "    * ASSERT FAILED: (%s), file %s, line %d.\n", expr, file, line);
}

void test_string_eq(const char* expected, const char* actual, const char* file, int line)
{
    if ((expected == NULL && actual != NULL) ||
        (expected != NULL && actual == NULL) ||
        (expected != NULL && actual != NULL && strcmp(expected, actual) != 0)) {
        if (asserts_failed == 0)
            printf("[\033[91mFAILED\033[0m]\n");
        asserts_failed++;
        fprintf(stderr, "    * ASSERT FAILED: (string equality), file %s, line %d.\n", file, line);
        fprintf(stderr, "      Expected: \"%s\"\n", expected ? expected : "NULL");
        fprintf(stderr, "      Actual:   \"%s\"\n", actual ? actual : "NULL");
    }
}

void test_lox_string_eq(const char* expected, Value actual, const char* file, int line)
{
    ObjString* actual_str = NULL;
    if (IS_STRING(actual)) {
        actual_str = AS_STRING(actual);
    }

    if (actual_str == NULL || strcmp(expected, actual_str->chars) != 0) {
        if (asserts_failed == 0)
            printf("[\033[91mFAILED\033[0m]\n");
        asserts_failed++;
        fprintf(stderr, "    * ASSERT FAILED: (Lox string equality), file %s, line %d.\n", file, line);
    }

    if (asserts_failed > 0) {
        test_output_enabled = false;
        fprintf(stderr, "      Expected: \"%s\"\n", expected);
        fprintf(stderr, "      Actual:   ");
        print_value_debug(actual);
        fprintf(stderr, "\n");
        test_output_enabled = true;
    }
}

void test_lox_int_eq(int expected, Value actual, const char* file, int line)
{
    if (!IS_INT(actual) || AS_INT(actual) != expected) {
        test_output_enabled = false;
        if (asserts_failed == 0)
            printf("[\033[91mFAILED\033[0m]\n");
        asserts_failed++;
        fprintf(stderr, "    * ASSERT FAILED: (int), file %s, line %d.\n", file, line);
        fprintf(stderr, "      Expected: %d\n", expected);
        fprintf(stderr, "      Actual:   ");
        print_value_debug(actual);
        fprintf(stderr, "\n");
        test_output_enabled = true;
    }
}

