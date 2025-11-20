////////////////////////////////////////////////////////////////////////////////
// util_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"

#include <olc/string_edit.h>

#include <color.h>
#include <db.h>
#include <format.h>
#include <match.h>

TestGroup util_tests;

static int test_match_none()
{
    const char* p = "This is  a string";
    const char* s = "This  is a string";

    ASSERT(mini_match(p, s, '$'));

    const char* n = "This is the wrong string";

    ASSERT(!mini_match(p, n, '$'));

    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_match_escaped()
{
    const char* p = "This is  a $$string";
    const char* s = "This   is a $string";

    ASSERT(mini_match(p, s, '$'));

    test_output_buffer = NIL_VAL;

    const char* n = "This is a $$string";

    ASSERT(!mini_match(p, n, '$'));

    return 0;
}

static int test_match_subs()
{
    const char* p = "$A $$$W =   $N; // 0x$X";
    const char* s = "var $blah_256  = 1234;   // 0x04d2";

    ASSERT(mini_match(p, s, '$'));

    const char* n = "va0 $123256  = 12a34; // 0x04g2";

    ASSERT(!mini_match(p, n, '$'));

    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_match_positive_closure()
{
    const char* p = "theme preview $W+ ";
    const char* s = "theme   preview    Cool Cat   ";

    ASSERT(mini_match(p, s, '$'));

    const char* n = "theme preview ";

    ASSERT(!mini_match(p, n, '$'));

    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_match_kleene_closure()
{
    const char* p = "theme preview $W* ";
    const char* s = "theme    preview ";

    ASSERT(mini_match(p, s, '$'));

    const char* n = "theme preview 1234";

    ASSERT(!mini_match(p, n, '$'));

    test_output_buffer = NIL_VAL;

    return 0;
}

void register_util_tests()
{
#define REGISTER(n, f)  register_test(&util_tests, (n), (f))

    init_test_group(&util_tests, "UTILITY TESTS");
    register_test_group(&util_tests);

    REGISTER("Pattern Matching: No Match", test_match_none);
    REGISTER("Pattern Matching: Escaped", test_match_escaped);
    REGISTER("Pattern Matching: Substitutions", test_match_subs);
    REGISTER("Pattern Matching: Positive Closure", test_match_positive_closure);
    REGISTER("Pattern Matching: Kleene Closure", test_match_kleene_closure);

#undef REGISTER
}
