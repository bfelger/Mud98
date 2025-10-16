///////////////////////////////////////////////////////////////////////////////
// test_registry.c
///////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"

#include <lox/value.h>
#include <lox/object.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern bool test_output_enabled;

TestGroup** test_groups = NULL;
size_t group_count = 0;
size_t group_capacity = 0;

void register_test_group(TestGroup* group)
{
    if (group_count >= group_capacity) {
        size_t new_capacity = group_capacity == 0 ? 8 : group_capacity * 2;
        TestGroup** new_groups = realloc(test_groups, new_capacity * sizeof(TestGroup*));
        if (!new_groups) {
            fprintf(stderr, "Failed to allocate memory for test group.\n");
            exit(1);
        }
        test_groups = new_groups;
        group_capacity = new_capacity;
    }

    test_groups[group_count] = group;
    group_count++;
}

void register_test(TestGroup* group, const char* name, TestFunc func)
{
    if (group->count >= group->capacity) {
        size_t new_capacity = group->capacity == 0 ? 8 : group->capacity * 2;
        TestCase* new_tests = realloc(group->tests, new_capacity * sizeof(TestCase));
        if (!new_tests) {
            fprintf(stderr, "Failed to allocate memory for test registry.\n");
            exit(1);
        }
        group->tests = new_tests;
        group->capacity = new_capacity;
    }

    group->tests[group->count].name = name;
    group->tests[group->count].func = func;
    group->tests[group->count].group = group;
    group->count++;
}


