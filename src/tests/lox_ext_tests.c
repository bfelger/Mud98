///////////////////////////////////////////////////////////////////////////////
// lox_ext_tests.c
///////////////////////////////////////////////////////////////////////////////

// Test generic language extensions to the Lox interpreter for Mud98.

#include "lox_tests.h"

#include <db.h>

#include <lox/array.h>
#include <lox/object.h>
#include <lox/scanner.h>
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
    ASSERT_LOX_OUTPUT_EQ("0\n100\n5\n13\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_array_add()
{
    const char* src =
        "var a = [];\n"
        "a.add(10);\n"
        "a.add(20);\n"
        "a.add(30);\n"
        "for (var i = 0; i < 3; i++)\n"
        "   print a[i];\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("10\n20\n30\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_array_contains()
{
    const char* src =
        "var a = [0, 1, 2, 3, 5, 8, 13, 21];\n"
        "print a.contains(13);\n"
        "print a.contains(7);\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("true\nfalse\n");

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

    ASSERT_LOX_OUTPUT_EQ("16\n32\n64\nstring\n17\n31\n65\nblah blah blah\n");

    pop(); // args

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_lamdas()
{
    const char* src =
        "var mob_count = 0\n"
        "global_mobs.each((mob) -> {\n"
        "    mob_count++\n"
        "})\n"
        "print \"Mobile Instance Count: \" + mob_count\n";
    
    interpret_code(src);
    int printed_count = 0;
    if (test_output_buffer && IS_STRING(test_output_buffer))
        sscanf(AS_CSTRING(test_output_buffer), "Mobile Instance Count: %d", &printed_count);
    ASSERT(printed_count > 0);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_bare_lamdas()
{
    const char* src =
        "var blah = () -> { print \"blah!\" }\n"
        "blah()\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("blah!\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_lamda_values()
{
    const char* src =
        "fun do(a, b) {\n"
        "    print a\n"
        "    b()\n"
        "}\n"
        "do(\"blah\", () -> {\n"
        "    print \"duh\"\n"
        "})\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("blah\nduh\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_string_interp_escape()
{
    const char* src =
        "var a = 2\n"
        "print \"a = $$\"\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("a = $$\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_string_interp_var()
{
    // The string '$a' should not be interpolated.
    // We need to be able to continue using $ specifiers for act() messages.

    const char* src =
        "var a = 2\n"
        "print \"The value '$a' is stored in 'a'.\"\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("The value '$a' is stored in 'a'.\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_string_interp_expr()
{
    const char* src =
        "var a = 2\n"
        "var b = 3\n"
        "print \"The value '${a+b}' is stored in 'a+b'.\"\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("The value '5' is stored in 'a+b'.\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_string_interp_expr_scanning()
{
    // Run the scanner directly over a string with multiple interpolations and
    // verify the tokens come out as expected.
    const char* src = "\"The values are ${a+b} and ${b-a}.\"\n";
    init_scanner(src);
    Token token;
    token = scan_token();
    ASSERT(token.type == TOKEN_STRING_INTERP);
    ASSERT(strncmp(token.start, "\"The values are ", token.length) == 0);

    // Parse the ${a+b}
    token = scan_token();
    ASSERT(token.type == TOKEN_DOLLAR);
    token = scan_token();
    ASSERT(token.type == TOKEN_LEFT_BRACE);
    token = scan_token();
    ASSERT(token.type == TOKEN_IDENTIFIER);
    ASSERT(strncmp(token.start, "a", token.length) == 0);
    token = scan_token();
    ASSERT(token.type == TOKEN_PLUS);
    token = scan_token();
    ASSERT(token.type == TOKEN_IDENTIFIER);
    ASSERT(strncmp(token.start, "b", token.length) == 0);
    token = scan_token();
    ASSERT(token.type == TOKEN_RIGHT_BRACE);
    token = scan_token();

    // Now get interstitial string
    ASSERT(token.type == TOKEN_STRING_INTERP);
    ASSERT(strncmp(token.start, "} and ", token.length) == 0);

    // ${b-a}
    token = scan_token();
    ASSERT(token.type == TOKEN_DOLLAR);
    token = scan_token();
    ASSERT(token.type == TOKEN_LEFT_BRACE);
    token = scan_token();
    ASSERT(token.type == TOKEN_IDENTIFIER);
    ASSERT(strncmp(token.start, "b", token.length) == 0);
    token = scan_token();
    ASSERT(token.type == TOKEN_MINUS);
    token = scan_token();
    ASSERT(token.type == TOKEN_IDENTIFIER);
    ASSERT(strncmp(token.start, "a", token.length) == 0);
    token = scan_token();
    ASSERT(token.type == TOKEN_RIGHT_BRACE);
    token = scan_token();

    // The last bit should be a TOKEN_STRING, not a TOKEN_STRING_INTERP
    ASSERT(token.type == TOKEN_STRING);
    ASSERT(strncmp(token.start, "}.\"", token.length) == 0);
    token = scan_token();
    ASSERT(token.type == TOKEN_EOF);
    return 0;
}

static int test_string_interp_expr_2()
{
    const char* src =
        "var a = 2\n"
        "var b = 3\n"
        "print \"The values are ${a+b} and ${b-a}.\"\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("The values are 5 and 1.\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_string_interp_var_start()
{
    const char* src = "print \"$n places a hand on $S shouler.\"\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("$n places a hand on $S shouler.\n");

    test_output_buffer = NIL_VAL;
    return 0;

}

static int test_string_lit_concat()
{
    const char* src = "var str = \"This is a compound string. \"\n"
        "    \"I hope it prints!\"\n"
        "print str\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("This is a compound string. I hope it prints!\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_string_lit_interp_concat()
{
    const char* src =
        "var a = 5\n"
        "var b = 3\n"
        "var str = \"a + b = ${a + b}. \"\n"
        "          \"a - b = ${a - b}.\"\n"
        "print str\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("a + b = 8. a - b = 2.\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_const_1()
{
    // Place test consts in a local scope so they don't pollute global.
    // Using const in a REPL context is not advised.
    const char* src =
        "{\n"
        "    const a = 2\n"
        "    print a\n"
        "}\n";

    test_disassemble_on_test = true;

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("2\n"
        "== <script> ==\n"
        "0000    3 OP_CONSTANT         0 2\n"   // 'a' is defined on line 1, but not placed on the stack until line 2.
        "0002    | OP_PRINT\n"
        "0003    5 OP_NIL\n"
        "0004    | OP_RETURN\n"
        "\n");

    test_disassemble_on_test = false;

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_const_2()
{
    // Place test consts in a local scope so they don't pollute global.
    // Using const in a REPL context is not advised.
    const char* src =
        "{\n"
        "    const a = 2\n"
        "    var b = a + 1\n"
        "    print b\n"
        "}\n";

    test_disassemble_on_test = true;

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("3\n"
        "== <script> ==\n"
        "0000    3 OP_CONSTANT         0 2\n"
        "0002    | OP_CONSTANT         1 1\n"
        "0004    | OP_ADD\n"
        "0005    4 OP_GET_LOCAL        1\n"
        "0007    | OP_PRINT\n"
        "0008    5 OP_POP\n"
        "0009    6 OP_NIL\n"
        "0010    | OP_RETURN\n"
        "\n");

    test_disassemble_on_test = false;

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_const_folding()
{
    // Place test consts in a local scope so they don't pollute global.
    // Using const in a REPL context is not advised.
    const char* src =
        "{\n"
        "    const a = 2 * (3 + 1)\n"
        "    print a\n"
        "}\n";

    test_disassemble_on_test = true;

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("8\n"
        "== <script> ==\n"
        "0000    3 OP_CONSTANT         0 8\n"
        "0002    | OP_PRINT\n"
        "0003    5 OP_NIL\n"
        "0004    | OP_RETURN\n"
        "\n");

    test_disassemble_on_test = false;

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_enum_auto()
{
    // Place test enums in a local scope so they don't pollute global.
    // Using const in a REPL context is not advised.
    const char* src =
        "{\n"
        "    enum A { B, C, D, E }\n"
        "    print A.E\n"
        "}";

    test_disassemble_on_test = true;

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("3\n"
        "== <script> ==\n"
        "0000    3 OP_CONSTANT         0 3\n"
        "0002    | OP_PRINT\n"
        "0003    4 OP_NIL\n"
        "0004    | OP_RETURN\n"
        "\n");

    test_disassemble_on_test = false;

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_enum_assign()
{
    // Place test enums in a local scope so they don't pollute global.
    // Using const in a REPL context is not advised.
    const char* src =
        "{\n"
        "    enum A {\n"
        "       B = 5,"
        "       C = 82,"
        "       D = 105,"
        "       E = 927,"
        "    }\n"
        "    print A.E\n"
        "}\n";

    test_disassemble_on_test = true;

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("927\n"
        "== <script> ==\n"
        "0000    4 OP_CONSTANT         0 927\n"
        "0002    | OP_PRINT\n"
        "0003    6 OP_NIL\n"
        "0004    | OP_RETURN\n"
        "\n"
    );

    test_disassemble_on_test = false;

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_enum_bootval()
{
    // Test enums generated at boot time.
    // Example: DamageType from data/damage.c.
    char* src =
        "print DamageType.Bash\n"
        "print DamageType.Pierce\n"
        "print DamageType.Slash\n"
        "print DamageType.Fire\n"
        "print DamageType.Cold\n";
    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("1\n"
        "2\n"
        "3\n"
        "4\n"
        "5\n");
    test_output_buffer = NIL_VAL;

    src =
        "print Race.Human\n"
        "print Race.Elf\n"
        "print Race.Dwarf\n"
        "print Race.Giant\n";
    result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("1\n"
        "2\n"
        "3\n"
        "4\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_array_class_member()
{
    char* src =
        "class Blah {\n"
        "init() {\n"
        "    this.foo = [];\n"
        "}\n"
        "add(n) {\n"
        "this.foo.add(n);\n"
        "}\n"
        "}\n"
        "var blah = Blah();\n"
        "blah.add(2)\n"
        "print blah.foo[0]\n";
    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("2\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

void register_lox_ext_tests()
{
#define REGISTER(n, f)  register_test(&lox_ext_tests, (n), (f))

    init_test_group(&lox_ext_tests, "LOX EXTENSION TESTS");
    register_test_group(&lox_ext_tests);

    REGISTER("Arrays: Accessing Elements", test_array_access);
    REGISTER("Arrays: Adding Elements", test_array_add);
    REGISTER("Arrays: Searching for Elements", test_array_contains);
    REGISTER("Marshaled Raw Values", test_marshaled_raw_vals);
    REGISTER("Lamdas", test_lamdas);
    REGISTER("Bare Lamdas", test_bare_lamdas);
    REGISTER("Lamda Values", test_lamda_values);
    REGISTER("String Interpolation: Escape", test_string_interp_escape);
    REGISTER("String Interpolation: Act Vars", test_string_interp_var);
    REGISTER("String Interpolation: Expressions", test_string_interp_expr);
    REGISTER("String Interpolation: Expression Parsing", test_string_interp_expr_scanning);
    REGISTER("String Interpolation: Expressions (2)", test_string_interp_expr_2);
    REGISTER("String Interpolation: Starting Act Var", test_string_interp_var_start);
    REGISTER("String Literal Concatenation", test_string_lit_concat);
    REGISTER("String Interp + Literal Concatenation", test_string_lit_interp_concat);
    REGISTER("Constant Values #1", test_const_1);
    REGISTER("Constant Values #2", test_const_2);
    REGISTER("Constant Folding", test_const_folding);
    REGISTER("Enum: Auto-Increment", test_enum_auto);
    REGISTER("Enum: Assign", test_enum_assign);
    REGISTER("Enum: Boot Vals", test_enum_bootval);
    REGISTER("Array Class Member", test_array_class_member);

#undef REGISTER
}
