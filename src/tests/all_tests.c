///////////////////////////////////////////////////////////////////////////////
// all_tests.c
///////////////////////////////////////////////////////////////////////////////

#include "tests.h"

#include "test_registry.h"

#include "lox_tests.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

bool test_output_enabled = false;

void run_all_tests();

void run_unit_tests()
{
    register_lox_tests();
    register_lox_ext_tests();
    register_container_tests();
    register_entity_tests();
    register_act_tests();

    test_output_enabled = true;
    test_dissasemble_on_error = true;

    run_all_tests();

    test_output_enabled = false;
    test_dissasemble_on_error = false;
}