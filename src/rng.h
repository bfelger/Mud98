////////////////////////////////////////////////////////////////////////////////
// rng.h
// Random Number Generator abstraction layer.
//
// Provides a seam for swapping RNG implementations (production vs test mocks).
// This enables deterministic testing of combat, events, and other RNG-dependent
// systems without changing production code.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__RNG_H
#define MUD98__RNG_H

#include "data/direction.h"

#include <stdint.h>

// RNG operations table.
// Production code points to PCG implementation; tests can swap in mocks.
typedef struct rng_ops_t {
    // Generate random number in range [from, to] inclusive
    int (*number_range)(int from, int to);
    
    // Generate random percentage [1, 100]
    int (*number_percent)(void);
    
    // Generate random number in range [0, 2^width - 1]
    int (*number_bits)(int width);
    
    // Generate random door direction [0, MAX_DIR-1]
    Direction (*number_door)(void);
    
    // Generate "million" random number (game mechanics)
    long (*number_mm)(void);
    
    // Roll dice: returns sum of 'number' d'size' rolls
    int (*dice)(int number, int size);
    
    // Fuzzy number: add slight random variance to input
    int (*number_fuzzy)(int number);
} RngOps;

// Global RNG ops pointer.
// Defaults to production PCG implementation (rng_production.c).
// Tests can swap to mock implementations (tests/mock_rng.c).
extern RngOps* rng;

// Production RNG ops table (backed by PCG)
extern RngOps rng_production;

// Convenience macros for calling through RNG ops.
// These mirror the original function signatures for drop-in compatibility.
#define RNG_RANGE(from, to)     (rng->number_range((from), (to)))
#define RNG_PERCENT()           (rng->number_percent())
#define RNG_BITS(width)         (rng->number_bits(width))
#define RNG_DOOR()              (rng->number_door())
#define RNG_MM()                (rng->number_mm())
#define RNG_DICE(num, size)     (rng->dice((num), (size)))
#define RNG_FUZZY(number)       (rng->number_fuzzy(number))

// Legacy function names still available for gradual migration.
// New code should prefer RNG_* macros or rng-> direct calls.
static inline int number_range(int from, int to) { return rng->number_range(from, to); }
static inline int number_percent(void) { return rng->number_percent(); }
static inline int number_bits(int width) { return rng->number_bits(width); }
static inline Direction number_door(void) { return rng->number_door(); }
static inline long number_mm(void) { return rng->number_mm(); }
static inline int dice(int number, int size) { return rng->dice(number, size); }
static inline int number_fuzzy(int number) { return rng->number_fuzzy(number); }

#endif // !MUD98__RNG_H
