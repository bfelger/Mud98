///////////////////////////////////////////////////////////////////////////////
// lox_ext_tests.c
///////////////////////////////////////////////////////////////////////////////

// Test generic language extensions to the Lox interpreter for Mud98.

#include "lox_tests.h"

#include <db.h>

#include <lox/array.h>
#include <lox/object.h>
#include <lox/value.h>
#include <lox/vm.h>

TestGroup lox_ext_tests;

static int test_array_access()
{
    const char* src =
        "var a = [0, 1, 2, 3, 5, 8, 13, 21];\n"
        "a[2] = 100;\n"
        "for (var i = 0; i < 8; i += 2)\n"
        "   print a[i];\n";

    InterpretResult result = interpret_code(src);
    ASSERT_OUTPUT_EQ("0\n100\n5\n13\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_marshaled_raw_vals()
{    
    int16_t i16 = 16;
    int32_t i32 = 32;
    uint64_t u64 = 64;
    char* str = str_dup("string");
    
    const char* src =
        "fun test_interop(i16, i32, u64, str) {\n"
        "   print i16;\n"
        "   print i32;\n"
        "   print u64;\n"
        "   print str;\n"
        "   i16++;\n"
        "   i32--;\n"
        "   u64++;\n"
        "   str = \"blah blah blah\";\n"
        "}\n";

        InterpretResult result = interpret_code(src);
        ASSERT(result == INTERPRET_OK);
        
        ValueArray* args = new_obj_array();
        push(OBJ_VAL(&args)); // Protect args as we make them.
        
        Value raw_i16 = WRAP_I16(i16);
        write_value_array(args, raw_i16);
        
        Value raw_i32 = WRAP_I32(i32);
        write_value_array(args, raw_i32);
        
        Value raw_u64 = WRAP_U64(u64);
        write_value_array(args, raw_u64);
        
        Value raw_str = WRAP_STR(str);
        write_value_array(args, raw_str);
        
        result = call_function("test_interop", 4, raw_i16, raw_i32, raw_u64, raw_str);
        ASSERT(result == INTERPRET_OK);

        result = call_function("test_interop", 4, raw_i16, raw_i32, raw_u64, raw_str);
        ASSERT(result == INTERPRET_OK);

        ASSERT_OUTPUT_EQ("16\n32\n64\nstring\n17\n31\n65\nblah blah blah\n");

        pop(); // args

        test_output_buffer = NIL_VAL;
        return 0;
}

static int test_lambdas()
{
    const char* src =
        "var mob_count = 0\n"
        "global_mobs.each((mob) -> {\n"
        "    mob_count++\n"
        "})\n"
        "print \"Mobile Instance Count: \" + mob_count\n";
    
    InterpretResult result = interpret_code(src);
    ASSERT_OUTPUT_EQ("Mobile Instance Count: 2166\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

void register_lox_ext_tests()
{
#define REGISTER(n, f)  register_test(&lox_ext_tests, (n), (f))

    init_test_group(&lox_ext_tests, "LOX EXTENSION TESTS");
    register_test_group(&lox_ext_tests);

    REGISTER("Array Access and Mutation", test_array_access);
    REGISTER("Marshaled Raw Values", test_marshaled_raw_vals);
    REGISTER("Lambdas", test_lambdas);

#undef REGISTER
}