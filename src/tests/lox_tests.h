///////////////////////////////////////////////////////////////////////////////
// lox_tests.h
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TESTS__LOX_EXT_TESTS_H
#define MUD98__TESTS__LOX_EXT_TESTS_H

#include "tests.h"

#include "test_registry.h"

#include <lox/object.h>
#include <lox/value.h>

#include <stdbool.h>

extern int asserts_failed;
extern bool test_disassemble_on_error;
extern bool test_disassemble_on_test;
extern Value test_output_buffer;

// Implicitly imported from lox/vm.c
bool invoke(ObjString* name, int arg_count);
void print_stack(void);

void register_lox_tests(void);
void register_lox_ext_tests(void);

#endif // !MUD98__TESTS__LOX_EXT_TESTS_H