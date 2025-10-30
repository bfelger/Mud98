////////////////////////////////////////////////////////////////////////////////
// fmt_test.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <olc/lox_edit.h>

#include <comm.h>
#include <db.h>

TestGroup fmt_tests = { 0 };

static int test_lox_edit()
{
    Mobile* pc = mock_player("Jim");
    pc->pcdata->current_theme = dup_color_theme(system_color_themes[SYSTEM_COLOR_THEME_LOPE]);
    SET_BIT(pc->act_flags, PLR_COLOUR);

    char* src =
        "// This is a comment.\n"
        "fun echo(n)\n"
        "{\n"
        "    print \"This is a string.\";\n"
        "    return n; // This is an in-line comment.\n"
        "}\n"
        "print echo(echo(1) + echo(2)) + echo(echo(4) + echo(5));\n";

    char* out = prettify_lox_script(src);

    char* expected = "{* 1{x. {6// This is a comment.\n\r"
        "{* 2{x. {|fun {xecho{_({xn{_)\n\r"
        "{* 3{x. {_{{\n\r"
        "{* 4{x.     {|print {=\"This is a string.\"{_;\n\r"
        "{* 5{x.     {|return {xn{_; {6// This is an in-line comment.\n\r"
        "{* 6{x. {_}\n\r"
        "{* 7{x. {|print {xecho{_({xecho{_({91{_) + {xecho{_({92{_)) + {xecho{_({xecho{_({94{_) + {xecho{_({95{_));\n\r"
        "{* 8{x. ";

    ASSERT_STR_EQ(expected, out);
    test_output_buffer = NIL_VAL;

    // If you want to see how the Lox syntax highlighting looks on the console,
    // uncomment the code below.
    //char buf[MSL];
    //colourconv(buf, out, pc);
    //printf("%s\n", buf);

    // If a test fails, and you want to know where, uncommend the code below.
    //size_t len = strlen(out);
    //for (size_t i = 0; i < len; i++)
    //    if (out[i] != expected[i]) {
    //        printf("First diff @ character %d: %c (%02x) != %c (%0x2)\n", (int)i, out[i], (int)out[i], expected[i], (int)out[i]);
    //        printf("%s\n", &out[i]);
    //        break;
    //    }

    free_string(out);

    free_color_theme(pc->pcdata->current_theme);

    return 0;
}

static int test_lox_edit2()
{
    Mobile* pc = mock_player("Jim");
    pc->pcdata->current_theme = dup_color_theme(system_color_themes[SYSTEM_COLOR_THEME_LOPE]);
    SET_BIT(pc->act_flags, PLR_COLOUR);

    char* src =
        "// This function gets called the first time a character logs in as an elf, after character creation.\n\r"
        "on_login(vch) {\n\r"
        "    // Only newly-created characters\n\r"
        "    if (vch.level > 1 or vch.race != Race.Elf)\n\r"
        "        return;\n\r"
        "\n\r"
        "    vch.send(\n\r"
        "        \"{*You step off the end of an old rope ladder at the base of Cuivealda, the Tree of Awakening. Your years in the nurturing hands of the Tetyayath have come to an end.{/{/Now you must go out into the world and find your destiny...{/{x\")\n\r"
        "\n\r"
        "    if (vch.can_quest(12000)) {\n\r"
        "        delay(1, () -> {\n\r"
        "            vch.send(\"{jYou have instructions to meet your old mentor in a clearing just to the east. There you will continue your training. Type '{*EXITS{j' to see what lies in that  direction. Type '{*EAST{j' to go there.{x\")\n\r"
        "            vch.grant_quest(12000)\n\r"
        "            vch.send(\"{jType '{*QUEST{j' to view your quest log.{x\")\n\r"
        "        })\n\r"
        "    }\n\r"
        "}\n\r";

        char* out = prettify_lox_script(src);

    char* expected = "{* 1{x. {6// This function gets called the first time a character logs in as an elf, after character creation.\n\r"
            "{* 2{x. on_login{_({xvch{_) {{\n\r"
            "{* 3{x.     {6// Only newly-created characters\n\r"
            "{* 4{x.     {|if {_({xvch{_.{xlevel {_> {91 {|or {xvch{_.{xrace {_!= {xRace{_.{xElf{_)\n\r"
            "{* 5{x.         {|return{_;\n\r"
            "{* 6{x. \n\r"
            "{* 7{x.     vch{_.{xsend{_(\n\r"
            "{* 8{x.         {=\"{{*You step off the end of an old rope ladder at the base of Cuivealda, the Tree of Awakening. Your years in the nurturing hands of the Tetyayath have come to an end.{{/{{/Now you must go out into the world and find your destiny...{{/{{x\"{_)\n\r"
            "{* 9{x. \n\r"
            "{*10{x.     {|if {_({xvch{_.{xcan_quest{_({912000{_)) {{\n\r"
            "{*11{x.         delay{_({91{_, () -> {{\n\r"
            "{*12{x.             vch{_.{xsend{_({=\"{{jYou have instructions to meet your old mentor in a clearing just to the east. There you will continue your training. Type '{{*EXITS{{j' to see what lies in that  direction. Type '{{*EAST{{j' to go there.{{x\"{_)\n\r"
            "{*13{x.             vch{_.{xgrant_quest{_({912000{_)\n\r"
            "{*14{x.             vch{_.{xsend{_({=\"{{jType '{{*QUEST{{j' to view your quest log.{{x\"{_)\n\r"
            "{*15{x.         {_})\n\r"
            "{*16{x.     {_}\n\r"
            "{*17{x. {_}\n\r"
            "{*18{x. ";

    ASSERT_STR_EQ(expected, out);
    test_output_buffer = NIL_VAL;

    // If you want to see how the Lox syntax highlighting looks on the console,
    // uncomment the code below.
    //char buf[MSL];
    //colourconv(buf, out, pc);
    //printf("%s\n", buf);
    
    // If a test fails, and you want to know where, uncommend the code below.
    //size_t len = strlen(out);
    //for (size_t i = 0; i < len; i++)
    //    if (out[i] != expected[i]) {
    //        printf("First diff @ character %d: %c (%02x) != %c (%0x2)\n", (int)i, out[i], (int)out[i], expected[i], (int)out[i]);
    //        printf("%s\n", &out[i]);
    //        break;
    //    }

    free_string(out);

    free_color_theme(pc->pcdata->current_theme);

    return 0;
}

void register_fmt_tests()
{
#define REGISTER(n, f)  register_test(&fmt_tests, (n), (f))

    init_test_group(&fmt_tests, "FORMATTING TESTS");

    register_test_group(&fmt_tests);

    REGISTER("Lox Syntax Highlighting Test #1", test_lox_edit);
    REGISTER("Lox Syntax Highlighting Test #2", test_lox_edit2);

#undef REGISTER
}
