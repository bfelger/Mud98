///////////////////////////////////////////////////////////////////////////////
// tests.c
///////////////////////////////////////////////////////////////////////////////

#include "tests.h"

#include "mock.h"

#include <lox/object.h>

#include <match.h>
#include <stdio.h>
#include <string.h>

extern TestGroup** test_groups;
extern size_t group_count;
extern size_t group_capacity;

void reset_stack();

static TestGroup* current = NULL;

int total_failures = 0;
int total_tests = 0;

void init_test_group(TestGroup* group, const char* name)
{
    group->name = name;
    group->tests = NULL;
    group->capacity = 0;
    group->count = 0;
    group->asserts_failed = 0;
}

static void run_test_group(TestGroup* group)
{
    current = group;

    int len = (int)strlen(group->name);

    printf("\n%.*s\n", len + 4, "*******************************************");
    printf("* %s *\n", group->name);
    printf("%.*s\n", len + 4, "*******************************************");

    printf("Running %zu tests:\n", group->count);

    int failures = 0;
    for (size_t i = 0; i < group->count; i++) {
        printf("Test: %-40s", group->tests[i].name);
        group->tests[i].func();
        reset_stack();
        if (group->asserts_failed == 0) {
            printf("[\033[92mPASSED\033[0m]\n");
        }
        else {
            group->asserts_failed = 0;
            failures++;
            total_failures++;
        }
        total_tests++;
        test_output_buffer = NIL_VAL;
    }
    printf("%d out of %d tests failed.\n", (int)failures, (int)group->count);
}

void run_all_tests()
{
    total_failures = 0;

    for (size_t i = 0; i < group_count; i++)
    {
        run_test_group(test_groups[i]);
    }

    printf("\nTOTAL: %d out of %d tests failed.\n\n", (int)total_failures, (int)total_tests);
}

void test_fail(const char* expr, const char* file, int line)
{
    if (current->asserts_failed == 0)
        printf("[\033[91mFAILED\033[0m]\n");
    current->asserts_failed++;
    fprintf(stderr, "    * ASSERT FAILED: (%s), file %s, line %d.\n", expr, file, line);
}

void test_string_eq(const char* expected, const char* actual, const char* file, int line)
{
    if ((expected == NULL && actual != NULL) ||
        (expected != NULL && actual == NULL) ||
        (expected != NULL && actual != NULL && strcmp(expected, actual) != 0)) {
        if (current->asserts_failed == 0)
            printf("[\033[91mFAILED\033[0m]\n");
        current->asserts_failed++;
        fprintf(stderr, "    * ASSERT FAILED: (string equality), file %s, line %d.\n", file, line);
        fprintf(stderr, "      Expected: \"%s\"\n", expected ? expected : "NULL");
        fprintf(stderr, "      Actual:   \"%s\"\n", actual ? actual : "NULL");
    }
}

void test_string_match(const char* pattern, const char* actual, const char* file, int line)
{
    if ((pattern == NULL && actual != NULL) ||
        (pattern != NULL && actual == NULL) ||
        (pattern != NULL && actual != NULL && !mini_match(pattern, actual, '$'))) {
        if (current->asserts_failed == 0)
            printf("[\033[91mFAILED\033[0m]\n");
        current->asserts_failed++;
        fprintf(stderr, "    * ASSERT FAILED: (pattern match), file %s, line %d.\n", file, line);
        fprintf(stderr, "      Pattern:  \"%s\"\n", pattern ? pattern : "NULL");
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
        if (current->asserts_failed == 0)
            printf("[\033[91mFAILED\033[0m]\n");
        current->asserts_failed++;
        fprintf(stderr, "    * ASSERT FAILED: (Lox string equality), file %s, line %d.\n", file, line);
    }

    if (current->asserts_failed > 0) {
        test_output_enabled = false;
        fprintf(stderr, "      Expected: \"%s\"\n", expected);
        fprintf(stderr, "      Actual:   ");
        print_value_debug(actual);
        fprintf(stderr, "\n");
        test_output_enabled = true;
    }
}

void test_lox_string_contains(const char* substring, Value actual, const char* file, int line)
{
    ObjString* actual_str = NULL;
    if (IS_STRING(actual)) {
        actual_str = AS_STRING(actual);
    }
    if (actual_str == NULL || strstr(actual_str->chars, substring) == NULL) {
        if (current->asserts_failed == 0)
            printf("[\033[91mFAILED\033[0m]\n");
        current->asserts_failed++;
        fprintf(stderr, "    * ASSERT FAILED: (Lox string contains), file %s, line %d.\n", file, line);
    }
    if (current->asserts_failed > 0) {
        test_output_enabled = false;
        fprintf(stderr, "      Expected to contain: \"%s\"\n", substring);
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
        if (current->asserts_failed == 0)
            printf("[\033[91mFAILED\033[0m]\n");
        current->asserts_failed++;
        fprintf(stderr, "    * ASSERT FAILED: (int), file %s, line %d.\n", file, line);
        fprintf(stderr, "      Expected: %d\n", expected);
        fprintf(stderr, "      Actual:   ");
        print_value_debug(actual);
        fprintf(stderr, "\n");
        test_output_enabled = true;
    }
}

static char safe_buf[MSL];
char* safe_arg(char* arg)
{
    sprintf(safe_buf, "%s", arg != NULL ? arg : "NULL");
    return safe_buf;
}

