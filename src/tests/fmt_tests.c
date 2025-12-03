////////////////////////////////////////////////////////////////////////////////
// fmt_test.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <olc/lox_edit.h>

#include <comm.h>
#include <db.h>
#include <format.h>

TestGroup fmt_tests = { 0 };

// LOX FORMAT TESTS ////////////////////////////////////////////////////////////

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

    char* expected =
        COLOR_ALT_TEXT_1 " 1" COLOR_CLEAR ". " COLOR_LOX_COMMENT "// This is a comment.\n\r"
        COLOR_ALT_TEXT_1 " 2" COLOR_CLEAR ". " COLOR_LOX_KEYWORD "fun " COLOR_CLEAR "echo" COLOR_LOX_OPERATOR "(" COLOR_CLEAR "n" COLOR_LOX_OPERATOR ")\n\r"
        COLOR_ALT_TEXT_1 " 3" COLOR_CLEAR ". " COLOR_LOX_OPERATOR "{\n\r"
        COLOR_ALT_TEXT_1 " 4" COLOR_CLEAR ".     " COLOR_LOX_KEYWORD "print " COLOR_LOX_STRING "\"This is a string.\"" COLOR_LOX_OPERATOR ";\n\r"
        COLOR_ALT_TEXT_1 " 5" COLOR_CLEAR ".     " COLOR_LOX_KEYWORD "return " COLOR_CLEAR "n" COLOR_LOX_OPERATOR "; " COLOR_LOX_COMMENT "// This is an in-line comment.\n\r"
        COLOR_ALT_TEXT_1 " 6" COLOR_CLEAR ". " COLOR_LOX_OPERATOR "}\n\r"
        COLOR_ALT_TEXT_1 " 7" COLOR_CLEAR ". " COLOR_LOX_KEYWORD "print " COLOR_CLEAR "echo" COLOR_LOX_OPERATOR "(" COLOR_CLEAR "echo" COLOR_LOX_OPERATOR "(" COLOR_LOX_LITERAL "1" COLOR_LOX_OPERATOR ") + " COLOR_CLEAR "echo" COLOR_LOX_OPERATOR "(" COLOR_LOX_LITERAL "2" COLOR_LOX_OPERATOR ")) + " COLOR_CLEAR "echo" COLOR_LOX_OPERATOR "(" COLOR_CLEAR "echo" COLOR_LOX_OPERATOR "(" COLOR_LOX_LITERAL "4" COLOR_LOX_OPERATOR ") + " COLOR_CLEAR "echo" COLOR_LOX_OPERATOR "(" COLOR_LOX_LITERAL "5" COLOR_LOX_OPERATOR "));\n\r"
        COLOR_ALT_TEXT_1 " 8" COLOR_CLEAR ". ";

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
    //        printf("First diff @ character %d: '%c' (%02x) != '%c' (%02x)\n", (int)i, out[i], (int)out[i], expected[i], (int)expected[i]);
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
        "        \"" COLOR_ALT_TEXT_1 "You step off the end of an old rope ladder at the base of Cuivealda, the Tree of Awakening. Your years in the nurturing hands of the Tetyayath have come to an end." COLOR_NEWLINE COLOR_NEWLINE "Now you must go out into the world and find your destiny..." COLOR_NEWLINE COLOR_CLEAR "\")\n\r"
        "\n\r"
        "    if (vch.can_quest(12000)) {\n\r"
        "        delay(1, () -> {\n\r"
        "            vch.send(\"" COLOR_INFO "You have instructions to meet your old mentor in a clearing just to the east. There you will continue your training. Type '" COLOR_ALT_TEXT_1 "EXITS" COLOR_INFO "' to see what lies in that  direction. Type '" COLOR_ALT_TEXT_1 "EAST" COLOR_INFO "' to go there." COLOR_CLEAR "\")\n\r"
        "            vch.grant_quest(12000)\n\r"
        "            vch.send(\"" COLOR_INFO "Type '" COLOR_ALT_TEXT_1 "QUEST" COLOR_INFO "' to view your quest log." COLOR_CLEAR "\")\n\r"
        "        })\n\r"
        "    }\n\r"
        "}\n\r";

        char* out = prettify_lox_script(src);

        char* expected =
            COLOR_ALT_TEXT_1 " 1" COLOR_CLEAR ". " COLOR_LOX_COMMENT "// This function gets called the first time a character logs in as an elf, after character creation.\n\r"
            COLOR_ALT_TEXT_1 " 2" COLOR_CLEAR ". on_login" COLOR_LOX_OPERATOR "(" COLOR_CLEAR "vch" COLOR_LOX_OPERATOR ") {\n\r"
            COLOR_ALT_TEXT_1 " 3" COLOR_CLEAR ".     " COLOR_LOX_COMMENT "// Only newly-created characters\n\r"
            COLOR_ALT_TEXT_1 " 4" COLOR_CLEAR ".     " COLOR_LOX_KEYWORD "if " COLOR_LOX_OPERATOR "(" COLOR_CLEAR "vch" COLOR_LOX_OPERATOR "." COLOR_CLEAR "level " COLOR_LOX_OPERATOR "> " COLOR_LOX_LITERAL "1 " COLOR_LOX_KEYWORD "or " COLOR_CLEAR "vch" COLOR_LOX_OPERATOR "." COLOR_CLEAR "race " COLOR_LOX_OPERATOR "!= " COLOR_CLEAR "Race" COLOR_LOX_OPERATOR "." COLOR_CLEAR "Elf" COLOR_LOX_OPERATOR ")\n\r"
            COLOR_ALT_TEXT_1 " 5" COLOR_CLEAR ".         " COLOR_LOX_KEYWORD "return" COLOR_LOX_OPERATOR ";\n\r"
            COLOR_ALT_TEXT_1 " 6" COLOR_CLEAR ". \n\r"
            COLOR_ALT_TEXT_1 " 7" COLOR_CLEAR ".     vch" COLOR_LOX_OPERATOR "." COLOR_CLEAR "send" COLOR_LOX_OPERATOR "(\n\r"
            COLOR_ALT_TEXT_1 " 8" COLOR_CLEAR ".         " COLOR_LOX_STRING "\"" COLOR_ESC_STR COLOR_ALT_TEXT_1 "You step off the end of an old rope ladder at the base of Cuivealda, the Tree of Awakening. Your years in the nurturing hands of the Tetyayath have come to an end." COLOR_ESC_STR COLOR_NEWLINE "" COLOR_ESC_STR COLOR_NEWLINE "Now you must go out into the world and find your destiny..." COLOR_ESC_STR COLOR_NEWLINE "" COLOR_ESC_STR COLOR_CLEAR "\"" COLOR_LOX_OPERATOR ")\n\r"
            COLOR_ALT_TEXT_1 " 9" COLOR_CLEAR ". \n\r"
            COLOR_ALT_TEXT_1 "10" COLOR_CLEAR ".     " COLOR_LOX_KEYWORD "if " COLOR_LOX_OPERATOR "(" COLOR_CLEAR "vch" COLOR_LOX_OPERATOR "." COLOR_CLEAR "can_quest" COLOR_LOX_OPERATOR "(" COLOR_LOX_LITERAL "12000" COLOR_LOX_OPERATOR ")) {\n\r"
            COLOR_ALT_TEXT_1 "11" COLOR_CLEAR ".         delay" COLOR_LOX_OPERATOR "(" COLOR_LOX_LITERAL "1" COLOR_LOX_OPERATOR ", () -> {\n\r"
            COLOR_ALT_TEXT_1 "12" COLOR_CLEAR ".             vch" COLOR_LOX_OPERATOR "." COLOR_CLEAR "send" COLOR_LOX_OPERATOR "(" COLOR_LOX_STRING "\"" COLOR_ESC_STR COLOR_INFO "You have instructions to meet your old mentor in a clearing just to the east. There you will continue your training. Type '" COLOR_ESC_STR COLOR_ALT_TEXT_1 "EXITS" COLOR_ESC_STR COLOR_INFO "' to see what lies in that  direction. Type '" COLOR_ESC_STR COLOR_ALT_TEXT_1 "EAST" COLOR_ESC_STR COLOR_INFO "' to go there." COLOR_ESC_STR COLOR_CLEAR "\"" COLOR_LOX_OPERATOR ")\n\r"
            COLOR_ALT_TEXT_1 "13" COLOR_CLEAR ".             vch" COLOR_LOX_OPERATOR "." COLOR_CLEAR "grant_quest" COLOR_LOX_OPERATOR "(" COLOR_LOX_LITERAL "12000" COLOR_LOX_OPERATOR ")\n\r"
            COLOR_ALT_TEXT_1 "14" COLOR_CLEAR ".             vch" COLOR_LOX_OPERATOR "." COLOR_CLEAR "send" COLOR_LOX_OPERATOR "(" COLOR_LOX_STRING "\"" COLOR_ESC_STR COLOR_INFO "Type '" COLOR_ESC_STR COLOR_ALT_TEXT_1 "QUEST" COLOR_ESC_STR COLOR_INFO "' to view your quest log." COLOR_ESC_STR COLOR_CLEAR "\"" COLOR_LOX_OPERATOR ")\n\r"
            COLOR_ALT_TEXT_1 "15" COLOR_CLEAR ".         " COLOR_LOX_OPERATOR "})\n\r"
            COLOR_ALT_TEXT_1 "16" COLOR_CLEAR ".     " COLOR_LOX_OPERATOR "}\n\r"
            COLOR_ALT_TEXT_1 "17" COLOR_CLEAR ". " COLOR_LOX_OPERATOR "}\n\r"
            COLOR_ALT_TEXT_1 "18" COLOR_CLEAR ". ";

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
    //        printf("First diff @ character %d: %c (%02x) != %c (%02x)\n", (int)i, out[i], (int)out[i], expected[i], (int)expected[i]);
    //        printf("%s\n", &out[i]);
    //        break;
    //    }

    free_string(out);

    free_color_theme(pc->pcdata->current_theme);

    return 0;
}

// FORMAT_STRING TESTS /////////////////////////////////////////////////////////

static int test_string_format_large_text()
{
    const char* lorum_ipsum = "at vero eos et accusamus et iusto odio dignissimos "
        "ducimus qui blanditiis praesentium voluptatum deleniti atque corrupti "
        "quos dolores et quas molestias excepturi sint occaecati cupiditate non "
        "provident, similique sunt in culpa qui officia deserunt mollitia animi, "
        "id est laborum et dolorum fuga. et harum quidem rerum facilis est et "
        "expedita distinctio.Nam libero tempore, cum soluta nobis est eligendi "
        "optio cumque nihil impedit quo minus id quod maxime placeat facere "
        "possimus, omnis voluptas assumenda est, omnis dolor repellendus."
        "temporibus autem quibusdam et aut officiis debitis aut rerum "
        "necessitatibus saepe eveniet ut et voluptates repudiandae sint et "
        "molestiae non recusandae.itaque earum rerum hic tenetur a sapiente "
        "delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut "
        "perferendis doloribus asperiores repellat.";

    const char* expected =
        "At vero eos et accusamus et iusto odio dignissimos ducimus qui blanditiis\n\r"
        "praesentium voluptatum deleniti atque corrupti quos dolores et quas\n\r"
        "molestias excepturi sint occaecati cupiditate non provident, similique sunt\n\r"
        "in culpa qui officia deserunt mollitia animi, id est laborum et dolorum\n\r"
        "fuga.  Et harum quidem rerum facilis est et expedita distinctio.  Nam libero\n\r"
        "tempore, cum soluta nobis est eligendi optio cumque nihil impedit quo minus\n\r"
        "id quod maxime placeat facere possimus, omnis voluptas assumenda est, omnis\n\r"
        "dolor repellendus.  Temporibus autem quibusdam et aut officiis debitis aut\n\r"
        "rerum necessitatibus saepe eveniet ut et voluptates repudiandae sint et\n\r"
        "molestiae non recusandae.  Itaque earum rerum hic tenetur a sapiente\n\r"
        "delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut\n\r"
        "perferendis doloribus asperiores repellat.\n\r";

    char* out = format_string(lorum_ipsum);

    ASSERT_STR_EQ(expected, out);

    // If a test fails, and you want to know where, uncommend the code below.
    //size_t len = strlen(out);
    //for (size_t i = 0; i < len; i++)
    //    if (out[i] != expected[i]) {
    //        printf("First diff @ character %d: %c (%02x) != %c (%02x)\n", (int)i, out[i], (int)out[i], expected[i], (int)expected[i]);
    //        printf("%s\n", &out[i]);
    //        break;
    //    }

    free_string(out);

    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_string_format_bare_newlines()
{
    const char* lorum_ipsum = "at vero eos et accusamus et iusto odio dignissimos "
        "ducimus qui blanditiis praesentium\n\rvoluptatum deleniti atque corrupti "
        "quos dolores et quas molestias excepturi sint occaecati cupiditate non "
        "provident, similique sunt in culpa qui officia deserunt mollitia animi, "
        "id est laborum et dolorum fuga.\n\r\n\ret harum quidem rerum facilis est et "
        "expedita distinctio.\n\r\n\rNam libero tempore, cum soluta nobis est eligendi "
        "optio cumque nihil impedit quo minus id quod maxime placeat facere "
        "possimus, omnis voluptas assumenda est, omnis dolor repellendus.\n\r\n\r"
        "temporibus autem\n\rquibusdam et aut officiis debitis aut rerum "
        "necessitatibus saepe eveniet ut et voluptates repudiandae sint et "
        "molestiae non recusandae.\n\r\n\ritaque earum rerum hic tenetur a sapiente "
        "delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut "
        "perferendis doloribus asperiores repellat.";

    const char* expected =
        "At vero eos et accusamus et iusto odio dignissimos ducimus qui blanditiis\n\r"
        "praesentium voluptatum deleniti atque corrupti quos dolores et quas\n\r"
        "molestias excepturi sint occaecati cupiditate non provident, similique sunt\n\r"
        "in culpa qui officia deserunt mollitia animi, id est laborum et dolorum\n\r"
        "fuga.\n\r"
        "\n\r"
        "Et harum quidem rerum facilis est et expedita distinctio.\n\r"
        "\n\r"
        "Nam libero tempore, cum soluta nobis est eligendi optio cumque nihil impedit\n\r"
        "quo minus id quod maxime placeat facere possimus, omnis voluptas assumenda\n\r"
        "est, omnis dolor repellendus.\n\r"
        "\n\r"
        "Temporibus autem quibusdam et aut officiis debitis aut rerum necessitatibus\n\r"
        "saepe eveniet ut et voluptates repudiandae sint et molestiae non recusandae.\n\r"
        "\n\r"
        "Itaque earum rerum hic tenetur a sapiente delectus, ut aut reiciendis\n\r"
        "voluptatibus maiores alias consequatur aut perferendis doloribus asperiores\n\r"
        "repellat.\n\r";

    char* out = format_string(lorum_ipsum);

    ASSERT_STR_EQ(expected, out);

    // If a test fails, and you want to know where, uncommend the code below.
    //size_t len = strlen(out);
    //for (size_t i = 0; i < len; i++)
    //    if (out[i] != expected[i]) {
    //        printf("First diff @ character %d: %c (%02x) != %c (%02x)\n", (int)i, out[i], (int)out[i], expected[i], (int)expected[i]);
    //        printf("%s\n", &out[i]);
    //        break;
    //    }

    free_string(out);

    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_string_format_para_indent()
{
    const char* lorum_ipsum = "    at vero eos et accusamus et iusto odio dignissimos "
        "ducimus qui blanditiis praesentium voluptatum deleniti atque corrupti "
        "quos dolores et quas molestias excepturi sint occaecati cupiditate non "
        "provident, similique sunt in culpa qui officia deserunt mollitia animi, "
        "id est laborum et dolorum fuga.\n\r    et harum quidem rerum facilis est et "
        "expedita distinctio.\n\r\tNam libero tempore, cum soluta nobis est eligendi "
        "optio cumque nihil impedit quo minus id quod maxime placeat facere "
        "possimus, omnis voluptas assumenda est, omnis dolor repellendus.\n\r"
        "    temporibus autem quibusdam et aut officiis debitis aut rerum "
        "necessitatibus saepe eveniet ut et voluptates repudiandae sint et "
        "molestiae non recusandae.\n\r\titaque earum rerum hic tenetur a sapiente "
        "delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut "
        "perferendis doloribus asperiores repellat.";

    const char* expected =
        "    At vero eos et accusamus et iusto odio dignissimos ducimus qui\n\r"
        "blanditiis praesentium voluptatum deleniti atque corrupti quos dolores et\n\r"
        "quas molestias excepturi sint occaecati cupiditate non provident, similique\n\r"
        "sunt in culpa qui officia deserunt mollitia animi, id est laborum et dolorum\n\r"
        "fuga.\n\r"
        "    Et harum quidem rerum facilis est et expedita distinctio.\n\r"
        "    Nam libero tempore, cum soluta nobis est eligendi optio cumque nihil\n\r"
        "impedit quo minus id quod maxime placeat facere possimus, omnis voluptas\n\r"
        "assumenda est, omnis dolor repellendus.\n\r"
        "    Temporibus autem quibusdam et aut officiis debitis aut rerum\n\r"
        "necessitatibus saepe eveniet ut et voluptates repudiandae sint et molestiae\n\r"
        "non recusandae.\n\r"
        "    Itaque earum rerum hic tenetur a sapiente delectus, ut aut reiciendis\n\r"
        "voluptatibus maiores alias consequatur aut perferendis doloribus asperiores\n\r"
        "repellat.\n\r";

    char* out = format_string(lorum_ipsum);

    ASSERT_STR_EQ(expected, out);

    // If a test fails, and you want to know where, uncommend the code below.
    //size_t len = strlen(out);
    //for (size_t i = 0; i < len; i++)
    //    if (out[i] != expected[i]) {
    //        printf("First diff @ character %d: %c (%02x) != %c (%02x)\n", (int)i, out[i], (int)out[i], expected[i], (int)expected[i]);
    //        printf("%s\n", &out[i]);
    //        break;
    //    }

    free_string(out);

    test_output_buffer = NIL_VAL;

    return 0;
}

void register_fmt_tests()
{
#define REGISTER(n, f)  register_test(&fmt_tests, (n), (f))

    init_test_group(&fmt_tests, "FORMATTING TESTS");

    register_test_group(&fmt_tests);

    REGISTER("Lox Syntax Highlighting Test #1", test_lox_edit);
    REGISTER("Lox Syntax Highlighting Test #2", test_lox_edit2);
    REGISTER("String Format: Large Text", test_string_format_large_text);
    REGISTER("String Format: Bare Newlines", test_string_format_bare_newlines);
    REGISTER("String Format: Indented Paragraphs", test_string_format_para_indent);

#undef REGISTER
}
