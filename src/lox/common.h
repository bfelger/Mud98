////////////////////////////////////////////////////////////////////////////////
// common.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Lox's NAN-boxing technique uses type punning. This is explicitly UB, but 
// nevertheless works on all tested compilers. If it makes you uncomfortable,
// disable this.
#define NAN_BOXING

//#define DEBUG_PRINT_CODE
//#define DEBUG_TRACE_EXECUTION
//#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC

#define UINT8_COUNT (UINT8_MAX + 1)

// Externals
void* alloc_mem(size_t sMem);
void bug(const char* fmt, ...);
void free_mem(void* pMem, size_t sMem);

#endif
