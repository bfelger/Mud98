///////////////////////////////////////////////////////////////////////////////
// lox_tests.c
///////////////////////////////////////////////////////////////////////////////

#include "tests.h"

#include "lox_tests.h"

#include "test_registry.h"

#include <db.h>

#include <entities/area.h>
#include <entities/room.h>

#include <lox/lox.h>
#include <lox/value.h>
#include <lox/vm.h>

static TestGroup lox_tests;

bool test_dissasemble_on_error = false;

// Capture output from print statements
Value test_output_buffer = NIL_VAL;

static int test_masks_strings()
{
    Value s1 = OBJ_VAL(copy_string("hello", 5));
    ASSERT_LOX_STR_EQ("hello", OBJ_VAL(s1));

    return 0;
}

static int test_math_negation_1()
{
    Value a = INT_VAL(-4);
    
    ASSERT(IS_INT(a));
    ASSERT_LOX_INT_EQ(-4, a);

    int b = AS_INT(a);

    ASSERT(-4 == b);
    return 0;
}

static int test_math_negation_2()
{
    const char* src = "print -4;";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("-4\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_15_2_value_stack_1()
{
    const char* src = "print 5 - 3;";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("2\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_15_2_value_stack_2()
{
    const char* src = 
        "fun echo(n)\n"
        "{\n"
        "    print n;\n"
        "    return n;\n"
        "}\n"
        "print echo(echo(1) + echo(2)) + echo(echo(4) + echo(5));";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("1\n2\n3\n4\n5\n9\n12\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_17_challenge_1()
{
    const char* src =
        "print (-1 + 2) * 3 - -4";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("7\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_18_4_1_logical_not()
{
    const char* src =
        "print !true;\n"
        "print !false;\n"
        "print !!true;\n"
        "print !!false;\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("false\ntrue\ntrue\nfalse\n")
        ;
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_19_4_string_comp()
{
      const char* src =
        "print \"abc\" == \"abd\";\n"
        "print \"abc\" != \"abd\";\n"
        "print \"abc\" == \"abc\";\n"
        "print \"abc\" != \"abc\";\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("false\ntrue\ntrue\nfalse\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_19_5_string_concat()
{
      const char* src =
        "print \"abc\" + \"def\";\n"
        "var s = \"hello\";\n"
        "s = s + \" world\";\n"
        "print s;\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("abcdef\nhello world\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_21_0_late_binding()
{
    const char* src =
        "fun test() {\n"
        "   print late_bind();\n"
        "}\n"
        "fun late_bind() {\n"
        "   return \"late bound!\";\n"
        "}\n"
        "test();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("late bound!\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_21_3_reading_vars()
{
    const char* src = 
        "var beverage = \"cafe au lait\";\n"
        "var breakfast = \"beignets with \" + beverage;\n"
        "print breakfast;";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("beignets with cafe au lait\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_21_4_assigning_vars()
{
    const char* src =
        "var breakfast = \"beignets\";\n"
        "var beverage = \"cafe au lait\";\n"
        "breakfast = \"beignets with \" + beverage;\n"
        "print breakfast;\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("beignets with cafe au lait\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_22_scoped_vars()
{
    const char* src =
        "var x = \"global x\";\n"
        "fun f() {\n"
        "   print x;\n"
        "   var x = \"local x\";\n"
        "   print x;\n"
        "}\n"
        "f();\n"
        "print x;\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("global x\nlocal x\nglobal x\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_23_1_if_else()
{
    const char* src =
        "var x = 10;\n"
        "if (x < 5) {\n"
        "   print \"less than 5\";\n"
        "}\n"
        "else if (x < 10) {\n"
        "   print \"less than 10\";\n"
        "}\n"
        "else {\n"
        "   print \"10 or more\";\n"
        "}\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("10 or more\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_23_2_logical_operators()
{
    const char* src =
        "var x = 10;\n"
        "if (x > 5 and x < 15) {\n"
        "   print \"between 5 and 15\";\n"
        "}\n"
        "if (x < 5 or x > 15) {\n"
        "   print \"outside 5 to 15\";\n"
        "}\n"
        "else {\n"
        "   print \"inside 5 to 15\";\n"
        "}\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("between 5 and 15\ninside 5 to 15\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_23_3_while_loop()
{
    const char* src =
        "var count = 0;\n"
        "while (count < 5) {\n"
        "   print count;\n"
        "   count = count + 1;\n"
        "}\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("0\n1\n2\n3\n4\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_23_4_for_loop()
{
    const char* src =
        "for (var i = 0; i < 5; i = i + 1) {\n"
        "   print i;\n"
        "}\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("0\n1\n2\n3\n4\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_24_5_1_func_arg_binding()
{
    const char* src =
        "fun sum(a, b, c) {\n"
        "   return a + b + c;\n"
        "}\n"
        "print 4 + sum(5, 6, 7);\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("22\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_25_1_closure_objects()
{
    const char* src =
        "fun make_closure(value) {\n"
        "    fun closure()\n"
        "    {\n"
        "        print value;\n"
        "    }\n"
        "    return closure;\n"
        "}\n"
        "var doughnut = make_closure(\"doughnut\");\n"
        "var bagel = make_closure(\"bagel\");\n"
        "doughnut();\n"
        "bagel();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("doughnut\nbagel\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_25_2_closure_captures()
{
    const char* src =
        "fun make_counter() {\n"
        "    var i = 0;\n"
        "    fun closure() {\n"
        "        i = i + 1;\n"
        "        print i;\n"
        "    }\n"
        "    return closure;\n"
        "}\n"
        "var counter = make_counter();\n"
        "counter();\n"
        "counter();\n"
        "counter();\n";
    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("1\n2\n3\n");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_25_2_upvalues()
{
    const char* src =
        "fun outer() {\n"
        "    var x = 1;\n"
        "    x = 2;\n"
        "    fun inner()\n"
        "    {\n"
        "        print x;\n"
        "    }\n"
        "    inner();\n"
        "}\n"
        "outer()\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("2\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_25_2_2_flattened_upvalues()
{
    const char* src =
        "fun outer()\n"
        "{\n"
        "    var x = \"value\";\n"
        "    fun middle()\n"
        "    {\n"
        "        fun inner()\n"
        "        {\n"
        "            print x;\n"
        "        }\n"
        "\n"
        "        print \"create inner closure\";\n"
        "        return inner;\n"
        "    }\n"
        "    print \"return from outer\";\n"
        "    return middle;\n"
        "}\n"
        "var mid = outer();\n"
        "var in = mid();\n"
        "in();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("return from outer\ncreate inner closure\nvalue\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_25_3_upvalue_captures()
{
    const char* src =
        "fun outer()\n"
        "{\n"
        "    var x = \"before\";\n"
        "    fun inner()\n"
        "    {\n"
        "        x = \"assigned\";\n"
        "    }\n"
        "    inner();\n"
        "    print x;\n"
        "}\n"
        "outer();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("assigned\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_25_3_1_upvalues_in_closures()
{
    const char* src =
        "fun outer()\n"
        "{\n"
        "    var x = \"outside\";\n"
        "    fun inner()\n"
        "    {\n"
        "        print x;\n"
        "    }\n"
        "    inner();\n"
        "}\n"
        "outer();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("outside\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_25_4_closed_upvalues()
{
    const char* src =
        "fun outer()\n"
        "{\n"
        "    var x = \"outside\";\n"
        "    fun inner()\n"
        "    {\n"
        "        print x;\n"
        "    }\n"
        "    return inner;\n"
        "}\n"
        "var closure = outer();\n"
        "closure();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("outside\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_25_4_1_closed_upvals_closures()
{
    const char* src =
        "var globalSet;\n"
        "var globalGet;\n"
        "fun main()\n"
        "{\n"
        "    var a = \"initial\";\n"
        "\n"
        "    fun set() { a = \"updated\"; }\n"
        "    fun get() { print a; }\n"
        "\n"
        "    globalSet = set;\n"
        "    globalGet = get;\n"
        "}\n"
        "main();\n"
        "globalSet();\n"
        "globalGet();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("updated\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_27_4_1_setter_value()
{
    const char* src =
        "class Toast {}\n"
        "var toast = Toast();\n"
        "print toast.jam = \"grape\";\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("grape\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_27_4_1_implicit_properties()
{
    const char* src =
        "class Pair {}\n"
        "var pair = Pair();\n"
        "pair.first = 1;\n"
        "pair.second = 2;\n"
        "print pair.first + pair.second;\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("3\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_28_2_method_references()
{
    const char* src =
        "class Person {\n"
        "    say_name()\n"
        "    {\n"
        "        print this.name;\n"
        "    }\n"
        "}\n"
        "var jane = Person();\n"
        "jane.name = \"Jane\";\n"
        "var method = jane.say_name;\n"
        "method();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("Jane\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_28_2_3_calling_methods()
{
    const char* src =
        "class Scone {\n"
        "    topping(first, second)\n"
        "    {\n"
        "        print \"scone with \" + first + \" and \" + second;\n"
        "    }\n"
        "}\n"
        "var scone = Scone();\n"
        "scone.topping(\"berries\", \"cream\");\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("scone with berries and cream\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_28_4_initializers()
{
    const char* src =
        "class CoffeeMaker {\n"
        "    init(coffee)\n"
        "    {\n"
        "        this.coffee = coffee;\n"
        "    }\n"
        "    brew()\n"
        "    {\n"
        "        print \"Enjoy your cup of \" + this.coffee;\n"
        "        // No reusing the grounds!\n"
        "        this.coffee = nil;\n"
        "    }\n"
        "}\n"
        "var maker = CoffeeMaker(\"coffee and chicory\");\n"
        "maker.brew();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("Enjoy your cup of coffee and chicory\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_29_1_inheriting_methods()
{
    const char* src =
        "class Doughnut {\n"
        "    cook()\n"
        "    {\n"
        "        print \"Dunk in the fryer.\";\n"
        "    }\n"
        "}\n"
        "class Cruller < Doughnut {\n"
        "    finish()\n"
        "    {\n"
        "        print \"Glaze with icing.\";\n"
        "    }\n"
        "}\n"
        "var a = Cruller();\n"
        "a.cook();\n"
        "a.finish();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("Dunk in the fryer.\nGlaze with icing.\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_29_2_super_call_dispatch()
{
    const char* src =
        "class A {\n"
        "    method() { print \"A.method\"; }\n"
        "}\n"
        "class B < A {\n"
        "    method() { print \"B.method\"; super.method(); }\n"
        "}\n"
        "class C < B {\n"
        "    method() { print \"C.method\"; super.method(); }\n"
        "}\n"
        "var c = C();\n"
        "c.method();\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("C.method\nB.method\nA.method\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_29_3_super_closures()
{
    const char* src =
        "class A {\n"
        "    method()\n"
        "    {\n"
        "        print \"A method\";\n"
        "    }\n"
        "}\n"
        "class B < A{\n"
        "    method()\n"
        "    {\n"
        "        print \"B method\";\n"
        "    }\n"
        "    test()\n"
        "    {\n"
        "        super.method();\n"
        "    }\n"
        "}\n"
        "class C < B{}\n"
        "C().test()\n";

    InterpretResult result = interpret_code(src);
    ASSERT_LOX_OUTPUT_EQ("A method\n");

    test_output_buffer = NIL_VAL;
    return 0;
}

void register_lox_tests()
{
#define REGISTER(n, f)  register_test(&lox_tests, (n), (f))

    init_test_group(&lox_tests, "LOX TESTS");
    register_test_group(&lox_tests);

    REGISTER("Value Masks: Strings", test_masks_strings);
    REGISTER("Math Test: Negation 1", test_math_negation_1);
    REGISTER("Math Test: Negation 2", test_math_negation_2);
    REGISTER("15.2 Value Stack Math Test", test_15_2_value_stack_1);
    REGISTER("15.2 Value Stack Echo Test", test_15_2_value_stack_2);
    REGISTER("17 Challenge 1 Math Precedence", test_17_challenge_1);
    REGISTER("18.4.1 Logical Not", test_18_4_1_logical_not);
    REGISTER("19.4 String Comparison", test_19_4_string_comp);
    REGISTER("19.5 String Concatenation", test_19_5_string_concat);
    REGISTER("21.0 Late Binding", test_21_0_late_binding);
    REGISTER("21.3 Reading Variables", test_21_3_reading_vars);
    REGISTER("21.4 Assigning Variables", test_21_4_assigning_vars);
    REGISTER("22 Scoped Variables", test_22_scoped_vars);
    REGISTER("23.1 If-Else", test_23_1_if_else);
    REGISTER("23.2 Logical Operators", test_23_2_logical_operators);
    REGISTER("23.3 While Loop", test_23_3_while_loop);
    REGISTER("23.4 For Loop", test_23_4_for_loop);
    REGISTER("24.5.1 Function Arg Binding", test_24_5_1_func_arg_binding);
    REGISTER("25.1 Closure Objects", test_25_1_closure_objects);
    REGISTER("25.2 Closure Captures", test_25_2_closure_captures);
    REGISTER("25.2 Upvalues", test_25_2_upvalues);
    REGISTER("25.2.2 Flattened Upvalues", test_25_2_2_flattened_upvalues);
    REGISTER("25.3 Upvalue Captures", test_25_3_upvalue_captures);
    REGISTER("25.3.1 Upvalues in Closures", test_25_3_1_upvalues_in_closures);
    REGISTER("25.4 Closed Upvalues", test_25_4_closed_upvalues);
    REGISTER("25.4.1 Closed Upvalues in Closures", test_25_4_1_closed_upvals_closures);
    REGISTER("27.4.1 Setter Value", test_27_4_1_setter_value);
    REGISTER("27.4.1 Implicit Properties", test_27_4_1_implicit_properties);
    REGISTER("28.2 Method References", test_28_2_method_references);
    REGISTER("28.2.3 Calling Methods", test_28_2_3_calling_methods);
    REGISTER("28.4 Initializers", test_28_4_initializers);
    REGISTER("29.1 Inheriting Methods", test_29_1_inheriting_methods);
    REGISTER("29.2 Super Call Dispatch", test_29_2_super_call_dispatch);
    REGISTER("29.3 Super in Closures", test_29_3_super_closures);

#undef REGISTER
}
