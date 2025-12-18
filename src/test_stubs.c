////////////////////////////////////////////////////////////////////////////////
// test_stubs.c
////////////////////////////////////////////////////////////////////////////////

// Test Globals Stub
//
// Provides stub definitions for test-related globals that are
// referenced throughout the core library but only used by test code.
// These stubs are compiled into the main executable to satisfy linker
// requirements while keeping test infrastructure out of production.

#include <lox/value.h>
#include <lox/array.h>
#include <stdbool.h>

// Test output control flags - always disabled in production
bool test_output_enabled = false;
bool test_act_output_enabled = false;
bool test_socket_output_enabled = false;
bool test_trace_exec = false;
bool test_disassemble_on_error = false;
bool test_disassemble_on_test = false;

// Test output buffer - never used in production
Value test_output_buffer = NIL_VAL;

// Mock entity array - never used in production
ValueArray* mocks_ = NULL;
