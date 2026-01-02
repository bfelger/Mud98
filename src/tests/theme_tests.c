////////////////////////////////////////////////////////////////////////////////
// tests/theme_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <color.h>
#include <command.h>
#include <db.h>
#include <olc/olc.h>
#include <recycle.h>

static TestGroup theme_tests;

static Color make_ansi(uint8_t light, uint8_t index)
{
    Color color = { 0 };
    color.mode = COLOR_MODE_16;
    color.code[0] = light;
    color.code[1] = index;
    return color;
}

static Color make_palette_ref(uint8_t slot)
{
    Color color = { 0 };
    color.mode = COLOR_MODE_PAL_IDX;
    color.code[0] = slot;
    return color;
}

static ColorTheme* build_system_theme(const char* name)
{
    ColorTheme* theme = new_color_theme();
    theme->name = str_dup(name);
    theme->banner = str_dup("");
    theme->mode = COLOR_MODE_16;
    theme->palette_max = 2;
    theme->palette[0] = make_ansi(NORMAL, WHITE);
    theme->palette[1] = make_ansi(NORMAL, BLACK);
    for (int i = 0; i < COLOR_SLOT_COUNT; ++i)
        theme->channels[i] = make_palette_ref(0);
    theme->channels[SLOT_BACKGROUND] = make_palette_ref(1);
    theme->type = COLOR_THEME_TYPE_SYSTEM;
    return theme;
}

static void register_system_themes(const char** names, size_t count)
{
    ASSERT(count > 0);

    ColorTheme** list = calloc(count, sizeof(ColorTheme*));
    ASSERT(list != NULL);

    for (size_t i = 0; i < count; ++i)
        list[i] = build_system_theme(names[i]);

    ASSERT(color_register_system_themes(list, (int)count));
}

static Mobile* build_theme_player(const char* name, bool sec9)
{
    Mobile* ch = mock_player(name);
    SET_BIT(ch->act_flags, PLR_COLOUR);
    ch->pcdata->security = sec9 ? 9 : 0;

    if (ch->pcdata->current_theme)
        free_color_theme(ch->pcdata->current_theme);

    ch->pcdata->current_theme = dup_color_theme(system_color_themes[0]);
    ch->pcdata->theme_config.current_theme_name = str_dup(system_color_themes[0]->name);
    return ch;
}

static int test_theme_create_personal_theme()
{
    const char* names[] = { "Midnight", "Sunrise" };
    register_system_themes(names, sizeof(names) / sizeof(names[0]));

    Mobile* ch = build_theme_player("ThemeUser", false);

    char cmd[] = "create ansi Custom";
    do_theme(ch, cmd);

    ASSERT(ch->pcdata->current_theme != NULL);
    ASSERT_STR_EQ("Custom", ch->pcdata->current_theme->name);
    ASSERT(ch->pcdata->color_themes[0] == ch->pcdata->current_theme);
    ASSERT(ch->pcdata->current_theme->mode == COLOR_MODE_16);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_theme_create_system_requires_security()
{
    const char* names[] = { "Midnight", "Sunrise" };
    register_system_themes(names, sizeof(names) / sizeof(names[0]));

    int original_count = system_color_theme_count;
    Mobile* ch = build_theme_player("Builder", false);

    char cmd[] = "create ansi system Solar";
    do_theme(ch, cmd);

    ASSERT(system_color_theme_count == original_count);
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_theme_create_system_theme()
{
    const char* names[] = { "Midnight", "Sunrise" };
    register_system_themes(names, sizeof(names) / sizeof(names[0]));

    Mobile* ch = build_theme_player("Builder", true);

    char cmd[] = "create rgb system Solar";
    do_theme(ch, cmd);

    ASSERT(system_color_theme_count == 3);
    ASSERT_STR_EQ("Solar", system_color_themes[2]->name);
    ASSERT(system_color_themes[2]->type == COLOR_THEME_TYPE_SYSTEM);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_theme_remove_system_blocks_last()
{
    const char* names[] = { "Lonely" };
    register_system_themes(names, sizeof(names) / sizeof(names[0]));

    Mobile* ch = build_theme_player("Builder", true);

    char cmd[] = "remove system Lonely";
    do_theme(ch, cmd);

    ASSERT(system_color_theme_count == 1);
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_theme_remove_system_theme()
{
    const char* names[] = { "Midnight", "Sunrise" };
    register_system_themes(names, sizeof(names) / sizeof(names[0]));

    Mobile* ch = build_theme_player("Builder", true);

    char cmd[] = "remove system Sunrise";
    do_theme(ch, cmd);

    ASSERT(system_color_theme_count == 1);
    ASSERT_STR_EQ("Midnight", system_color_themes[0]->name);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_theme_edit_system_sets_editor()
{
    const char* names[] = { "Midnight", "Sunrise" };
    register_system_themes(names, sizeof(names) / sizeof(names[0]));

    Mobile* ch = build_theme_player("Builder", true);
    ColorTheme* expected = (ColorTheme*)system_color_themes[1];

    char cmd[] = "edit system Sunrise";
    do_theme(ch, cmd);

    ASSERT(get_editor(ch->desc) == ED_THEME);
    ASSERT((ColorTheme*)get_pEdit(ch->desc) == expected);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_theme_set_system_default()
{
    const char* names[] = { "Midnight", "Sunrise" };
    register_system_themes(names, sizeof(names) / sizeof(names[0]));

    Mobile* ch = build_theme_player("Builder", true);

    char cmd[] = "set system default Sunrise";
    do_theme(ch, cmd);

    ASSERT_STR_EQ("Sunrise", system_color_themes[0]->name);

    test_output_buffer = NIL_VAL;
    return 0;
}

void register_theme_tests()
{
#define REGISTER(name, fn) register_test(&theme_tests, (name), (fn))

    init_test_group(&theme_tests, "THEME TESTS");
    register_test_group(&theme_tests);

    REGISTER("Create Personal", test_theme_create_personal_theme);
    REGISTER("Create System Requires Security", test_theme_create_system_requires_security);
    REGISTER("Create System", test_theme_create_system_theme);
    REGISTER("Remove System Blocks Last", test_theme_remove_system_blocks_last);
    REGISTER("Remove System", test_theme_remove_system_theme);
    REGISTER("Edit System", test_theme_edit_system_sets_editor);
    REGISTER("Set System Default", test_theme_set_system_default);

#undef REGISTER
}
