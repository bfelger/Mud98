////////////////////////////////////////////////////////////////////////////////
// tests/mock_rng.c
// Deterministic mock RNG for unit tests.
//
// Provides predictable random number generation for testing combat, events,
// and other RNG-dependent systems without flakiness.
////////////////////////////////////////////////////////////////////////////////

#include "mock.h"
#include "rng.h"

// Mock RNG state
static int mock_rng_sequence[] = {50, 75, 25, 100, 10, 90, 40, 60, 30, 70};
static int mock_rng_index = 0;

// Mock RNG implementations
static int mock_number_range(int from, int to);
static int mock_number_percent(void);
static int mock_number_bits(int width);
static Direction mock_number_door(void);
static long mock_number_mm(void);
static int mock_dice(int number, int size);
static int mock_number_fuzzy(int number);

// Mock RNG ops table
RngOps mock_rng = {
    .number_range = mock_number_range,
    .number_percent = mock_number_percent,
    .number_bits = mock_number_bits,
    .number_door = mock_number_door,
    .number_mm = mock_number_mm,
    .dice = mock_dice,
    .number_fuzzy = mock_number_fuzzy,
};

// Get next value from deterministic sequence
static int get_next_mock_value(void)
{
    int value = mock_rng_sequence[mock_rng_index];
    mock_rng_index = (mock_rng_index + 1) % (sizeof(mock_rng_sequence) / sizeof(mock_rng_sequence[0]));
    return value;
}

// Reset mock RNG to start of sequence
void reset_mock_rng(void)
{
    mock_rng_index = 0;
}

// Set custom deterministic sequence
void set_mock_rng_sequence(int* sequence, int length)
{
    // Not implemented yet - uses default sequence
    (void)sequence;
    (void)length;
}

// Mock implementations - return deterministic values

static int mock_number_range(int from, int to)
{
    if (from >= to)
        return from;
    
    // Map next mock value (0-100) to range [from, to]
    int value = get_next_mock_value();
    int range = to - from + 1;
    return from + (value % range);
}

static int mock_number_percent(void)
{
    int value = get_next_mock_value();
    // Clamp to [1, 100]
    if (value < 1) return 1;
    if (value > 100) return 100;
    return value;
}

static int mock_number_bits(int width)
{
    int value = get_next_mock_value();
    return value & ((1 << width) - 1);
}

static Direction mock_number_door(void)
{
    return (Direction)(get_next_mock_value() % 6);
}

static long mock_number_mm(void)
{
    return (long)get_next_mock_value();
}

static int mock_dice(int number, int size)
{
    // For tests, return middle of the range for predictability
    if (size == 0) return 0;
    if (size == 1) return number;
    
    // Return average roll
    return (number * (size + 1)) / 2;
}

static int mock_number_fuzzy(int number)
{
    // No fuzz in tests - return number as-is
    return UMAX(1, number);
}
