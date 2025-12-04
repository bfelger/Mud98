////////////////////////////////////////////////////////////////////////////////
// colors.c
// Functions to handle VT102 SGR colors
////////////////////////////////////////////////////////////////////////////////

#include "color.h"

#include "comm.h"
#include "config.h"
#include "db.h"
#include "digest.h"
#include "handler.h"
#include "save.h"
#include "vt.h"

#include <olc/olc.h>

#include <persist/theme/theme_persist.h>

#include <entities/descriptor.h>
#include <entities/player_data.h>

#include <data/mobile_data.h>
#include <data/player.h>

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void set_default_theme(Mobile* ch);

#define COLOR2STR(t, s, x)  color_to_str((ColorTheme*)t, (Color*)&t->channels[s], x)
#define BGCOLOR2STR(t, x)   bg_color_to_str((ColorTheme*)t, (Color*)&t->channels[SLOT_BACKGROUND], x)

#define IS_HEX(c)   ((c >= '0' && c <= '9') \
        || (c >= 'A' && c <='F')            \
        || (c >= 'a' && c <= 'f'))

static const char* theme_change_warning = COLOR_INFO "You have have made changes to your "
    "active theme that must either be saved with " COLOR_ALT_TEXT_1 "THEME SAVE" COLOR_INFO " or discarded "
    "from " COLOR_ALT_TEXT_1 "THEME EDIT" COLOR_INFO " using the " COLOR_ALT_TEXT_1 "DISCARD" COLOR_INFO " command." COLOR_CLEAR "\n\r";

char* bg_color_to_str(const ColorTheme* theme, const Color* color, bool xterm)
{
    static char buf[50];

    if (color->mode == COLOR_MODE_PAL_IDX) {
        if (color->code[0] >= theme->palette_max)
            return "";

        int slot = color->code[0];
        color = &theme->palette[slot];
    }

    switch (color->mode) {
    case COLOR_MODE_PAL_IDX:
        bug("bad color reference (PAL->PAL)");
        return "";
    case COLOR_MODE_16:
        if (color->code[0] == 0)
            sprintf(buf, "\033[4%dm", color->code[1]);
        else
            sprintf(buf, "\033[10%dm", color->code[1]);
        break;
    case COLOR_MODE_256:
        sprintf(buf, "\033[" SGR_MODE_BG ";" SGR_BITS_8 ";%dm",
            color->code[0]);
        break;
    case COLOR_MODE_RGB:
        // The first byte is a nullable index. Not needed.
        if (!xterm)
            sprintf(buf, "\033[" SGR_MODE_BG ":" SGR_BITS_24 "::%d:%d:%dm",
                color->code[0],
                color->code[1],
                color->code[2]);
        else
            sprintf(buf, "\033[" SGR_MODE_BG ";" SGR_BITS_24 ";%d;%d;%dm",
                color->code[0],
                color->code[1],
                color->code[2]);
        break;
    }

    return buf;
}

char* color_to_str(ColorTheme* theme, Color* color, bool xterm)
{
    char buf[MAX_INPUT_LENGTH];

    if (color->mode == COLOR_MODE_PAL_IDX) {
        if (color->code[0] >= theme->palette_max)
            return "";

        int slot = color->code[0];
        color = &theme->palette[slot];
    }

    if (color->mode == COLOR_MODE_RGB && xterm) {
        if (color->xterm != NULL)
            return color->xterm;
    }
    else if (color->cache != NULL)
        return color->cache;

    switch (color->mode) {
    case COLOR_MODE_PAL_IDX:
        bug("bad color reference (PAL->PAL)");
        return "";
    case COLOR_MODE_16:
        if (color->code[0])
            sprintf(buf, "\033[9%dm", color->code[1]);
        else
            sprintf(buf, "\033[3%dm", color->code[1]);
        break;
    case COLOR_MODE_256:
        sprintf(buf, "\033[" SGR_MODE_FG ";" SGR_BITS_8 ";%dm", 
            color->code[0]);
        break;
    case COLOR_MODE_RGB:
        // The first byte is a nullable index. Not needed.
        if (!xterm)
            sprintf(buf, "\033[" SGR_MODE_FG ":" SGR_BITS_24 "::%d:%d:%dm",
                color->code[0],
                color->code[1],
                color->code[2]);
        else
            sprintf(buf, "\033[" SGR_MODE_FG ";" SGR_BITS_24 ";%d;%d;%dm",
                color->code[0],
                color->code[1],
                color->code[2]);
        break;
    }

    if (color->mode == COLOR_MODE_RGB && xterm) {
        color->xterm = str_dup(buf);
        return color->xterm;
    }
    
    color->cache = str_dup(buf);
    return color->cache;
}

static void list_theme_entry(Mobile* ch, ColorTheme* theme, const char* owner, bool show_public)
{
    INIT_BUF(out, MAX_STRING_LENGTH);
    char buf[MAX_STRING_LENGTH];
    bool xterm = ch->pcdata->theme_config.xterm;

    char bg_code[50];
    char fg_code[50];
    char mode[50] = { 0 };

    switch (theme->mode) {
    case COLOR_MODE_16:
        sprintf(mode, "(ANSI) ");
        break;
    case COLOR_MODE_256:
        sprintf(mode, "(256 Color) ");
        break;
    case COLOR_MODE_RGB:
        sprintf(mode, "(24-Bit) ");
        break;
    case COLOR_MODE_PAL_IDX:
        sprintf(mode, "(error) ");
    }

    char public_buf[50] = { 0 };
    if (show_public && theme->is_public) {
        sprintf(public_buf, "%s (public)", COLOR2STR(theme, SLOT_ALT_TEXT_2, xterm));
    }

    // Cache codes
    sprintf(fg_code, "%s", COLOR2STR(theme, SLOT_TEXT, xterm));
    sprintf(bg_code, "%s", BGCOLOR2STR(theme, xterm));

    // First line, blank BG color
    addf_buf(out, "%s                                                                      " COLOR_EOL, bg_code);

    // Second line, name and first row of color boxes
    char name[50] = { 0 };
    strcat(name, theme->name);
    if (owner != NULL) {
        size_t pad = strlen(owner) + strlen(name) + 2;
        char owner_buf[150] = { 0 };
        snprintf(owner_buf, 150, "%s@%s%s", COLOR2STR(theme, SLOT_ALT_TEXT_1, xterm), owner, fg_code);
        sprintf(buf, "%s%s    %s %s", bg_code, fg_code, owner_buf, name);
        for (size_t i = pad; i < 30; ++i)
            strcat(buf, " ");
    }
    else
        sprintf(buf, "%s%s    %-30s", bg_code, fg_code, name);
    add_buf(out, buf);
    for (int i = 0; i < PALETTE_SIZE; i += 2) {
        if (i <= theme->palette_max)
            addf_buf(out, "%s    ", bg_color_to_str(theme, &theme->palette[i], xterm));
        else
            addf_buf(out, "%s    ", bg_code); 
    }
    addf_buf(out, "%s    " COLOR_EOL, bg_code);

    // Third line, mode type and second row of color boxes
    if (show_public && theme->is_public) {
        size_t pad = strlen(mode) + 9;
        sprintf(buf, "%s%s    %s%s", COLOR2STR(theme, SLOT_ALT_TEXT_1, xterm), bg_code, mode, public_buf);
        for (size_t i = pad; i < 30; ++i)
            strcat(buf, " ");
    }
    else
        sprintf(buf, "%s%s    %-30s", COLOR2STR(theme, SLOT_ALT_TEXT_1, xterm), bg_code, mode);
    add_buf(out, buf);
    for (int i = 1; i < PALETTE_SIZE; i += 2) {
        if (i <= theme->palette_max)
            sprintf(buf, "%s    ", bg_color_to_str(theme, &theme->palette[i], xterm));
        else
            sprintf(buf, "%s    ", bg_code);
        add_buf(out, buf);
    }
    addf_buf(out, "%s    " COLOR_EOL, bg_code);

    // Fourth line, blank BG color
    addf_buf(out, "%s                                                                      " COLOR_CLEAR "\n\r\n\r", bg_code);

    send_to_char(out->string, ch);

    free_buf(out);
}

void send_256_color_list(Mobile* ch)
{
    int i;
    int text;
    send_to_char(COLOR_INFO "You can choose from the following:" COLOR_EOL, ch);
    for (int row = 0; row < 16; ++row)
        for (int col = 0; col < 16; ++col) {
            i = row * 16 + col;
            text = 0;
            if (i == 0 || i == 8 || (i >= 16 && i < 22) || (i >= 232 && i < 240))
                text = 15;
            printf_to_char(ch,
                "\033[" SGR_MODE_BG ";" SGR_BITS_8 ";%dm"
                "\033[" SGR_MODE_FG ";" SGR_BITS_8 ";%dm"
                " %-3d" COLOR_CLEAR , i, text, i);
            if (col == 15)
                send_to_char("\n\r", ch);
        }
    send_to_char("\n\r", ch);
}

void send_ansi_color_list(Mobile* ch)
{
    send_to_char(COLOR_INFO "You can choose from the following:" COLOR_EOL, ch);
    for (int i = 0; i < 15; ++i) {
        printf_to_char(ch, "    %s%-10s" COLOR_CLEAR , ansi_palette[i].color.cache,
            ansi_palette[i].name);
        if ((i + 1) % 3 == 0)
            send_to_char("\n\r", ch);
    }
    send_to_char("\n\r", ch);
}

bool lookup_color(char* argument, Color* color, Mobile* ch)
{
    if (argument == NULL || !argument[0]) {
        send_to_char(COLOR_INFO "You must choose a color." COLOR_EOL, ch);
        if (ch->pcdata->current_theme->mode == COLOR_MODE_16)
            send_ansi_color_list(ch);
        else if (ch->pcdata->current_theme->mode == COLOR_MODE_256)
            send_256_color_list(ch);
        return false;
    }

    char color_arg[50];
    READ_ARG(color_arg);

    if (color_arg[0] == '#') {
        // Set it to an 24-bit RGB value.
        for (int i = 1; i <= 6; ++i) {
            if (!color_arg[i] || !IS_HEX(color_arg[i])) {
                send_to_char(COLOR_INFO "RGB color codes must be hexadecimal values of "
                    "the format " COLOR_ALT_TEXT_1 "#RRBBGG;" COLOR_INFO "." COLOR_EOL, ch);
                return false;
            }
        }

        if (color_arg[7] != ';' || color_arg[8] != '\0') {
            send_to_char(COLOR_INFO "RGB color codes must be hexadecimal values of "
                "the format " COLOR_ALT_TEXT_1 "#RRGGBB;" COLOR_INFO "." COLOR_EOL, ch);
            return false;
        }

        uint8_t byte_buf[10];
        hex_to_bin(byte_buf, color_arg + 1, 3);
        set_color_rgb(color, byte_buf[0], byte_buf[1], byte_buf[2]);
    }
    else if (!str_prefix(color_arg, "palette")) {
        // Set it to a theme palette color.
        char pal_arg[50];
        int idx;
        int pal_max = ch->pcdata->current_theme->palette_max;

        READ_ARG(pal_arg);

        if (!pal_arg[0] || !is_number(pal_arg)) {
            send_to_char(COLOR_INFO "A palette space must be specified by number "
                "from 0 to 15." COLOR_EOL, ch);
            return false;
        }

        idx = atoi(pal_arg);

        if (idx < 0 || idx > pal_max) {
            printf_to_char(ch, COLOR_INFO "A palette space must be specified by number "
                "in range of the palette (0 to %d)." COLOR_EOL, pal_max);
            return false;
        }

        set_color_palette_ref(color, (uint8_t)idx);
    }
    else {
        if (ch->pcdata->current_theme->mode == COLOR_MODE_RGB) {
            send_to_char(COLOR_INFO "RGB color codes must be hexadecimal values of "
                "the format " COLOR_ALT_TEXT_1 "#RRBBGG;" COLOR_INFO "." COLOR_CLEAR " or a palette reference.\n\r", ch);
            return false;
        }
        // Set it to an ANSI or indexed color.
        uint8_t code = 0;
        uint8_t bright = 0;
        bool found = false;
        int pal_max = ch->pcdata->current_theme->palette_max;

        for (int i = 0; i < pal_max; ++i) {
            if (!str_prefix(color_arg, ansi_palette[i].name)) {
                bright = ansi_palette[i].bright;
                code = ansi_palette[i].code;
                found = true;
                break;
            }
        }

        if (!found) {
            printf_to_char(ch, COLOR_INFO "Unknown ANSI color name '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "'.\n\r",
                color_arg);
            if (ch->pcdata->current_theme->mode == COLOR_MODE_16)
                send_ansi_color_list(ch);
            else if (ch->pcdata->current_theme->mode == COLOR_MODE_256)
                send_256_color_list(ch);
            return false;
        }

        set_color_ansi(color, bright, code);
    }

    return true;
}

static void clear_cached_codes(ColorTheme* theme)
{
    for (int i = 0; i < theme->palette_max; ++i) {
        free_string(theme->palette[i].cache);
        theme->palette[i].cache = NULL;
        free_string(theme->palette[i].xterm);
        theme->palette[i].xterm = NULL;
    }

    for (int i = 0; i < COLOR_SLOT_COUNT; ++i) {
        free_string(theme->channels[i].cache);
        theme->channels[i].cache = NULL;
        free_string(theme->channels[i].xterm);
        theme->channels[i].xterm = NULL;
    }
}

static char* skip_spaces(char* str)
{
    if (!str)
        return str;

    while (*str && isspace((unsigned char)*str))
        ++str;

    return str;
}

static ColorTheme* color_find_system_theme(const char* name, bool allow_prefix);
static int color_find_system_theme_index(const char* name, bool allow_prefix);
static bool color_append_system_theme(ColorTheme* theme);
static bool color_remove_system_theme(int index);
static void destroy_system_theme(ColorTheme* theme);

static void do_theme_config(Mobile* ch, char* argument)
{
    static const char* help =
        COLOR_INFO
        "USAGE: " COLOR_ALT_TEXT_1 "THEME CONFIG (ENABLE|DISABLE) (256|RGB|XTERM)" COLOR_INFO "\n\r"
        "\n\r"
        "Toggle certain themes, depending on your client's capabilities.\n\r"
        "Has the following arguments:\n\r"
        "       " COLOR_ALT_TEXT_1 "ENABLE    " COLOR_INFO "- Enables the given mode (on by default). \n\r"
        "       " COLOR_ALT_TEXT_1 "DISABLE   " COLOR_INFO "- Hide themes of the given mode from " COLOR_ALT_TEXT_1 "LIST" COLOR_INFO ".\n\r\n\r"
        "       " COLOR_ALT_TEXT_1 "256       " COLOR_INFO "- Show/hide 256-color extended index palettes.\n\r"
        "       " COLOR_ALT_TEXT_1 "RGB       " COLOR_INFO "- Show/hide 24-bit RGB palettes.\n\r"
        "       " COLOR_ALT_TEXT_1 "XTERM     " COLOR_INFO "- Use " COLOR_ALT_TEXT_2 "xterm" COLOR_INFO " 24-bit color codes instead of VT100. This\n\r"
        "                   is used by Linux terminals and TinTin++. If 24-bit themes\n\r"
        "                   don't appear for you, try this first.\n\r"
        "       " COLOR_ALT_TEXT_1 "RGB_HELP  " COLOR_CLEAR "- Show/hide long-winded message at the end of " COLOR_ALT_TEXT_1 "THEME_LIST" COLOR_INFO "."
        COLOR_CLEAR "\n\r\n\r";

    char cmd[MAX_INPUT_LENGTH];
    char opt[MAX_INPUT_LENGTH];
    bool enable = false;

    READ_ARG(cmd);
    if (!cmd[0]) {
        send_to_char(help, ch);
        return;
    }

    READ_ARG(opt);
    if (!opt[0]) {
        send_to_char(help, ch);
        return;
    }

    if (!str_cmp(cmd, "enable"))
        enable = true;
    else if (!str_cmp(cmd, "disable"))
        enable = false;
    else {
        send_to_char(help, ch);
        return;
    }

    if (!str_cmp(opt, "256")) {
        ch->pcdata->theme_config.hide_256 = !enable;
        printf_to_char(ch, COLOR_INFO "256-color mode is now %s.\n\r", 
            enable ? "enabled" : "disabled");
    }
    else if (!str_cmp(opt, "rgb")) {
        ch->pcdata->theme_config.hide_24bit = !enable;
        printf_to_char(ch, COLOR_INFO "24-bit color mode is now %s.\n\r",
            enable ? "enabled" : "disabled");
    }
    else if (!str_cmp(opt, "xterm")) {
        ch->pcdata->theme_config.xterm = enable;
        printf_to_char(ch, COLOR_INFO "xterm mode is now %s.\n\r",
            enable ? "enabled" : "disabled");
    }
    else if (!str_cmp(opt, "rgb_help")) {
        ch->pcdata->theme_config.hide_rgb_help = !enable;
        printf_to_char(ch, COLOR_INFO "RGB list help is now %s." COLOR_EOL,
            capitalize(opt), !enable ? "enabled" : "disabled");
        return;
    }
    else {
        send_to_char(help, ch);
    }
}

static ColorTheme* create_theme_template(const char* name, ColorMode mode)
{
    ColorTheme* theme = new_color_theme();
    theme->name = str_dup(name);
    theme->mode = mode;
    theme->banner = str_dup("");
    theme->palette_max = 16;
    theme->is_public = false;
    theme->is_changed = true;

    Color white;
    Color black;

    switch (mode) {
    default:
    case COLOR_MODE_16:
        white = (Color){ .mode = COLOR_MODE_16, .code = { NORMAL, WHITE, 0 }, .cache = NULL, .xterm = NULL };
        black = (Color){ .mode = COLOR_MODE_16, .code = { NORMAL, BLACK, 0 }, .cache = NULL, .xterm = NULL };
        break;
    case COLOR_MODE_256:
        white = (Color){ .mode = COLOR_MODE_256, .code = { WHITE, 0, 0 }, .cache = NULL, .xterm = NULL };
        black = (Color){ .mode = COLOR_MODE_256, .code = { BLACK, 0, 0 }, .cache = NULL, .xterm = NULL };
        break;
    case COLOR_MODE_RGB:
        white = (Color){ .mode = COLOR_MODE_RGB, .code = { 0xC0u, 0xC0u, 0xC0u }, .cache = NULL, .xterm = NULL };
        black = (Color){ .mode = COLOR_MODE_RGB, .code = { 0x00u, 0x00u, 0x00u }, .cache = NULL, .xterm = NULL };
        break;
    case COLOR_MODE_PAL_IDX:
        white = (Color){ .mode = COLOR_MODE_16, .code = { NORMAL, WHITE, 0 }, .cache = NULL, .xterm = NULL };
        black = (Color){ .mode = COLOR_MODE_16, .code = { NORMAL, BLACK, 0 }, .cache = NULL, .xterm = NULL };
        break;
    }

    theme->palette[0] = white;
    theme->palette[1] = black;
    for (int i = 2; i < PALETTE_SIZE; ++i)
        theme->palette[i] = white;

    Color fg_idx = (Color){ .mode = COLOR_MODE_PAL_IDX, .code = { 0, 0, 0 }, .cache = NULL, .xterm = NULL };
    Color bg_idx = (Color){ .mode = COLOR_MODE_PAL_IDX, .code = { 1, 0, 0 }, .cache = NULL, .xterm = NULL };
    for (int i = 0; i < COLOR_SLOT_COUNT; ++i)
        theme->channels[i] = fg_idx;
    theme->channels[SLOT_BACKGROUND] = bg_idx;

    return theme;
}

static void do_theme_create(Mobile* ch, char* argument)
{
    static const char* help =
        COLOR_INFO
        "USAGE: " COLOR_ALT_TEXT_1 "THEME CREATE (ANSI|256|RGB) <name>" COLOR_INFO "\n\r"
        "\n\r"
        "Creates a new theme of the requested type and sets it to the currently "
        "active theme. The default is a solid black background and soft-white "
        "foreground colors.\n\r"
        "\n\r"
        "All channel colors are assigned to the first palette index except "
        "background, which is set to the second palette index (black by "
        "default). These values can be changed." COLOR_EOL;

    char mode_arg[MAX_INPUT_LENGTH];
    ColorMode mode;

    READ_ARG(mode_arg);

    argument = skip_spaces(argument);

    if (!mode_arg[0]
        || (mode = lookup_color_mode(mode_arg)) < COLOR_MODE_16
        || mode > COLOR_MODE_RGB
        || !argument[0]) {
        send_to_char(help, ch);
        return;
    }
    char first_word[MAX_INPUT_LENGTH] = { 0 };
    size_t fw_len = 0;
    while (argument[fw_len] && !isspace((unsigned char)argument[fw_len])
        && fw_len < MAX_INPUT_LENGTH - 1) {
        first_word[fw_len] = argument[fw_len];
        ++fw_len;
    }
    first_word[fw_len] = '\0';

    bool create_system = false;
    const char* theme_name = argument;
    if (first_word[0] && !str_cmp(first_word, "system")) {
        create_system = true;
        theme_name = skip_spaces(argument + fw_len);
        if (!theme_name[0]) {
            send_to_char(COLOR_INFO "You must supply the name of the system theme to create." COLOR_EOL, ch);
            return;
        }
    }

    if (!create_system && ch->pcdata->current_theme && ch->pcdata->current_theme->is_changed) {
        send_to_char(theme_change_warning, ch);
        return;
    }

    size_t len = strlen(theme_name);
    if (len < 2 || len > 12) {
        send_to_char(COLOR_INFO "Color theme names must be between 2 and 12 characters long." COLOR_EOL, ch);
        return;
    }

    for (int i = 0; i < system_color_theme_count; ++i) {
        if (!system_color_themes[i])
            continue;
        if (!str_cmp(theme_name, system_color_themes[i]->name)) {
            send_to_char(COLOR_INFO "You cannot use the name of an existing system or "
                "personal theme." COLOR_EOL, ch);
            return;
        }
    }

    ColorTheme* theme = create_theme_template(theme_name, mode);

    if (create_system) {
        if (IS_NPC(ch) || ch->pcdata->security < 9) {
            send_to_char(COLOR_INFO "You don't have permission to create system themes." COLOR_EOL, ch);
            free_color_theme(theme);
            return;
        }

        if (!color_append_system_theme(theme)) {
            free_color_theme(theme);
            send_to_char(COLOR_INFO "Unable to create the system theme." COLOR_EOL, ch);
            return;
        }

        theme->type = COLOR_THEME_TYPE_SYSTEM;
        theme->is_changed = true;
        printf_to_char(ch, COLOR_INFO "System theme '%s' created. Use THEME EDIT SYSTEM %s to modify it and THEME SAVE SYSTEM to persist." COLOR_EOL,
            theme->name, theme->name);
        return;
    }

    int slot = -1;
    for (int i = 0; i < MAX_THEMES; ++i) {
        if (ch->pcdata->color_themes[i] == NULL) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        free_color_theme(theme);
        send_to_char(COLOR_INFO "You already have the maximum number of personal color "
            "themes. You must remove one with 'THEME REMOVE' first." COLOR_EOL, ch);
        return;
    }

    free_color_theme(ch->pcdata->current_theme);
    ch->pcdata->current_theme = theme;
    ch->pcdata->color_themes[slot] = theme;

    printf_to_char(ch, "New theme created: %s\n\r", theme->name);
}

bool theme_discard_personal(Mobile* ch)
{
    ColorTheme* current_theme = ch->pcdata->current_theme;
    char* theme_name = ch->pcdata->theme_config.current_theme_name;

    if (!current_theme) {
        send_to_char(COLOR_INFO "You do not have a theme selected to discard." COLOR_EOL, ch);
        return false;
    }

    if (!current_theme->is_changed && (!theme_name || !str_cmp(current_theme->name, theme_name))) {
        printf_to_char(ch, COLOR_INFO "Your current theme '%s' has no pending changes.\n\r",
            current_theme->name);
        return false;
    }

    ColorTheme* theme = lookup_color_theme(ch, theme_name);
    if (!theme) {
        printf_to_char(ch, COLOR_INFO "Can no longer find a theme named '%s'.\n\r",
            theme_name ? theme_name : "(unknown)");
        return false;
    }

    free_color_theme(current_theme);
    ch->pcdata->current_theme = dup_color_theme(theme);
    if (ch->desc && ch->desc->editor == ED_THEME)
        ch->desc->pEdit = (uintptr_t)ch->pcdata->current_theme;
    return true;
}

bool theme_save_personal(Mobile* ch)
{
    ColorTheme* theme = ch->pcdata->current_theme;
    char* theme_name = ch->pcdata->theme_config.current_theme_name;

    if (!theme) {
        send_to_char(COLOR_INFO "You do not have a theme selected to save." COLOR_EOL, ch);
        return false;
    }

    for (int i = 0; i < MAX_THEMES; ++i) {
        if (!ch->pcdata->color_themes[i])
            continue;
        if (!theme_name || !str_cmp(theme_name, ch->pcdata->color_themes[i]->name)) {
            free_color_theme(ch->pcdata->color_themes[i]);
            theme->is_changed = false;
            ch->pcdata->color_themes[i] = dup_color_theme(theme);
            free_string(ch->pcdata->theme_config.current_theme_name);
            ch->pcdata->theme_config.current_theme_name = str_dup(theme->name);
            save_char_obj(ch);
            return true;
        }
    }

    for (int i = 0; i < MAX_THEMES; ++i) {
        if (!ch->pcdata->color_themes[i]) {
            ch->pcdata->color_themes[i] = dup_color_theme(theme);
            theme->is_changed = false;
            free_string(theme_name);
            ch->pcdata->theme_config.current_theme_name = str_dup(theme->name);
            save_char_obj(ch);
            return true;
        }
    }

    send_to_char(COLOR_INFO "You have no free theme slots to save into." COLOR_EOL, ch);
    return false;
}

static void do_theme_list(Mobile* ch, char* argument)
{
    static const char* help =
        COLOR_INFO
        "USAGE: " COLOR_ALT_TEXT_1 "THEME LIST (ALL|SYSTEM|PUBLIC|PRIVATE)" COLOR_INFO "\n\r"
        "\n\r"
        "Lists the available themes. Has the following arguments:\n\r"
        "       " COLOR_ALT_TEXT_1 "ALL     " COLOR_INFO "- Show all available themes you can select from. (default option)\n\r"
        "       " COLOR_ALT_TEXT_1 "SYSTEM  " COLOR_INFO "- Only show built-in themes.\n\r"
        "       " COLOR_ALT_TEXT_1 "PUBLIC  " COLOR_INFO "- Only show themes made public by current, logged-in users.\n\r"
        "       " COLOR_ALT_TEXT_1 "PRIVATE " COLOR_INFO "- Only show your own personal themes." COLOR_EOL;

    char opt[MAX_INPUT_LENGTH] = { 0 };
    bool system = false;
    bool shared = false;
    bool priv = false;

    READ_ARG(opt);

    if (!opt[0] || !str_prefix(opt, "all")) {
        priv = true;
        system = true;
        shared = true;
    }
    else if (!str_prefix(opt, "system")) {
        system = true;
    }
    else if (!str_prefix(opt, "public")) {
        shared = true;
    }
    else if (!str_prefix(opt, "private")) {
        priv = true;
    }
    else {
        send_to_char(help, ch);
        return;
    }

    bool found = false;
    bool hide_256 = ch->pcdata->theme_config.hide_256;
    bool hide_24bit = ch->pcdata->theme_config.hide_24bit;

    if (priv) {
        for (int i = 0; i < MAX_THEMES; ++i) {
            if (ch->pcdata->color_themes[i] != NULL) {
                if ((hide_256 && ch->pcdata->color_themes[i]->mode == COLOR_MODE_256) ||
                    (hide_24bit && ch->pcdata->color_themes[i]->mode == COLOR_MODE_RGB))
                    continue;
                if (!found) {
                    send_to_char("\n\r" COLOR_TITLE "Your personal themes:\n\r", ch);
                    send_to_char(COLOR_DECOR_2 "==========================================="
                        "===========================" COLOR_EOL, ch);
                    found = true;
                }
                list_theme_entry(ch, ch->pcdata->color_themes[i], NULL, true);
            }
        }
    }

    if (system) {
        send_to_char(COLOR_TITLE "System themes:\n\r", ch);
        send_to_char(COLOR_DECOR_2 "======================================================="
            "===============" COLOR_EOL, ch);
        for (int i = 0; i < system_color_theme_count; ++i) {
            if (!system_color_themes[i])
                continue;
            if ((hide_256 && system_color_themes[i]->mode == COLOR_MODE_256) ||
                (hide_24bit && system_color_themes[i]->mode == COLOR_MODE_RGB))
                continue;
            list_theme_entry(ch, (ColorTheme*)system_color_themes[i], NULL, false);
        }
    }

    found = false;
    ColorTheme* theme = NULL;
    if (shared) {
        for (Descriptor* d = descriptor_list; d != NULL; NEXT_LINK(d)) {
            if (d == ch->desc && priv)
                continue;

            Mobile* wch;

            if (d->connected != CON_PLAYING || !can_see(ch, d->character))
                continue;

            wch = (d->original != NULL) ? d->original : d->character;

            if (!can_see(ch, wch))
                continue;

            for (int i = 0; i < MAX_THEMES; ++i) {
                theme = wch->pcdata->color_themes[i];
                if (!theme || !theme->is_public)
                    continue;

                if ((hide_256 && theme->mode == COLOR_MODE_256) ||
                    (hide_24bit && theme->mode == COLOR_MODE_RGB))
                    continue;

                if (!found) {
                    send_to_char("\n\r", ch);
                    send_to_char(COLOR_TITLE "Public themes:\n\r", ch);
                    send_to_char(COLOR_DECOR_2 "==========================================="
                        "===========================" COLOR_EOL, ch);
                    found = true;
                }
                list_theme_entry(ch, theme, NAME_STR(wch), false);
            }
        }
    }

    if (!ch->pcdata->theme_config.hide_rgb_help) {
        send_to_char(
            COLOR_INFO "If you do not see any colors for 24-bit themes:" COLOR_EOL
            "  " COLOR_ALT_TEXT_1 "*" COLOR_CLEAR " Check your client to make sure it supports 24-bit color.\n\r"
            "  " COLOR_ALT_TEXT_1 "*" COLOR_CLEAR " Your client may want " COLOR_ALT_TEXT_2 "xterm" COLOR_CLEAR " color codes instead of VT-102 SGR commands\n\r"
            "    (TinTin++, for example). Try " COLOR_ALT_TEXT_1 "THEME CONFIG ENABLE XTERM" COLOR_CLEAR " and see if that helps.\n\r"
            "  " COLOR_ALT_TEXT_1 "*" COLOR_CLEAR " Your client may not support them at all. If so, type " COLOR_ALT_TEXT_1 "THEME CONFIG DISABLE\n\r"
            "    RGB" COLOR_CLEAR " to hide them from the list of themes.\n\r"
            "  " COLOR_ALT_TEXT_1 "*" COLOR_CLEAR " Likewise for 256 color indexed themes: type " COLOR_ALT_TEXT_1 "THEME CONFIG DISABLE 256" COLOR_EOL
            "    to hide them.\n\r"
            "  " COLOR_ALT_TEXT_1 "*" COLOR_CLEAR " To hide this message in the future, type " COLOR_ALT_TEXT_1 "THEME CONFIG DISABLE RGB_HELP" COLOR_CLEAR ".\n\r"
            , ch);
    }
}

void theme_preview_theme(Mobile* ch, ColorTheme* theme)
{
#define COL_FMT(slot)   COLOR2STR(theme, (slot), xterm)
    if (!theme) {
        send_to_char(COLOR_INFO "No color theme is available to preview." COLOR_EOL, ch);
        return;
    }

    bool xterm = ch->pcdata->theme_config.xterm;

    INIT_BUF(buf, MSL);

    char* bg = BGCOLOR2STR(theme, xterm);

    addf_buf(buf, "%s\n\r", bg);

    addf_buf(buf, "%sWelcome to Mud98 0.90. Please do not feed the mobiles."
        "\n\r\n\r", COL_FMT(SLOT_TEXT));
    
    addf_buf(buf, "%sLimbo\n\r", COL_FMT(SLOT_ROOM_TITLE));

    addf_buf(buf,
        "%sYou are floating in a formless void, detached from all sensation of physical\n\r"
        "matter, surrounded by swirling glowing light, which fades into the relative\n\r"
        "darkness around you without any trace of edges or shadow.\n\r"
        "There is a \"No Tipping\" notice pinned to the darkness.\n\r\n\r",
        COL_FMT(SLOT_ROOM_TEXT));

    addf_buf(buf,
        "     %sA Random Title               %s+--------------------+\n\r"
        "%s===========================       %s| %sDo you like Boxes? %s|\n\r"
        "%sOLC Field #1     %s[ %s12345 %s] %sThe nam%s+--------------------+\n\r"
        "%sOLC Field #2     %s[ %s98765 %s] %sBlah blah blah blah\n\r\n\r",
        COL_FMT(SLOT_TITLE), COL_FMT(SLOT_DECOR_3), COL_FMT(SLOT_DECOR_2),
        COL_FMT(SLOT_DECOR_3), COL_FMT(SLOT_TEXT), COL_FMT(SLOT_DECOR_3),
        COL_FMT(SLOT_TEXT), COL_FMT(SLOT_DECOR_1), COL_FMT(SLOT_ALT_TEXT_1),
        COL_FMT(SLOT_DECOR_1), COL_FMT(SLOT_ALT_TEXT_2), COL_FMT(SLOT_DECOR_3),
        COL_FMT(SLOT_TEXT), COL_FMT(SLOT_DECOR_1), COL_FMT(SLOT_ALT_TEXT_1),
        COL_FMT(SLOT_DECOR_1), COL_FMT(SLOT_ALT_TEXT_2)
    );

    addf_buf(buf, "%sA dire wallabee is here.\n\r", COL_FMT(SLOT_ROOM_THINGS));
    
    addf_buf(buf, "%sYou have no unread notes.\n\r\n\r", COL_FMT(SLOT_INFO));
    
    addf_buf(buf, "%s<20/20hp 100/100m 100/100v 0gp/0sp [U]>\n\r\n\r", COL_FMT(SLOT_PROMPT));
    
    addf_buf(buf, "%sA dire wallabee says, '%sG'day, mate!%s'\n\r\n\r",
        COL_FMT(SLOT_SAY), COL_FMT(SLOT_SAY_TEXT),
        COL_FMT(SLOT_SAY));
    
    addf_buf(buf, "%sA dire wallabee tells you, '%sBut not really... TIME TO DIE!%s'\n\r\n\r",
        COL_FMT(SLOT_TELL), COL_FMT(SLOT_TELL_TEXT),
        COL_FMT(SLOT_TELL));
    
    addf_buf(buf, "%s<20/20hp 100/100m 100/100v 0gp/0sp [U]>\n\r\n\r", COL_FMT(SLOT_PROMPT));
    
    addf_buf(buf, "%sA dire wallabee's slash scratches you.\n\r", COL_FMT(SLOT_FIGHT_OHIT));
   
    addf_buf(buf, "%sYour slash grazes a dire wallabee.\n\r", COL_FMT(SLOT_FIGHT_YHIT));

    addf_buf(buf, "%sA dire wallabee is DEAD!!\n\r", COL_FMT(SLOT_FIGHT_DEATH));

    addf_buf(buf, "%sYou receive 199 experience points.\n\r", COL_FMT(SLOT_TEXT));

    addf_buf(buf, "%s\n\r" COLOR_EOL, bg);

    send_to_char(BUF(buf), ch);
    free_buf(buf);
#undef COL_FMT
}

static void do_theme_preview(Mobile* ch, char* argument)
{
    static const char* help =
        COLOR_INFO
        "USAGE: " COLOR_ALT_TEXT_1 "THEME PREVIEW (<theme name>)" COLOR_INFO "\n\r"
        "\n\r"
        "For a list of themes to preview, type '" COLOR_ALT_TEXT_1 "THEME LIST" COLOR_INFO "'." COLOR_EOL;

    if (argument && argument[0] && !str_cmp(argument, "help")) {
        send_to_char(help, ch);
        return;
    }

    ColorTheme* theme = ch->pcdata->current_theme;

    if (argument && argument[0]) {
        theme = lookup_color_theme(ch, argument);

        if (!theme) {
            printf_to_char(ch, COLOR_INFO "No color theme named '%s' could be found." COLOR_EOL,
                argument);
            return;
        }
    }

    if (!theme) {
        send_to_char(COLOR_INFO "You do not have a color theme active. Use "
            COLOR_ALT_TEXT_1 "THEME SELECT" COLOR_INFO " to choose one, or "
            COLOR_ALT_TEXT_1 "THEME CREATE" COLOR_INFO " to make a new one." COLOR_EOL, ch);
        return;
    }

    theme_preview_theme(ch, theme);
}

static void do_theme_remove(Mobile* ch, char* argument)
{
    argument = skip_spaces(argument);
    if (!argument || !argument[0]) {
        send_to_char(COLOR_INFO "You specify the name of a color theme to remove." COLOR_EOL,
            ch);
        return;
    }

    char first_word[MAX_INPUT_LENGTH] = { 0 };
    size_t fw_len = 0;
    while (argument[fw_len] && !isspace((unsigned char)argument[fw_len])
        && fw_len < MAX_INPUT_LENGTH - 1) {
        first_word[fw_len] = argument[fw_len];
        ++fw_len;
    }
    first_word[fw_len] = '\0';

    if (first_word[0] && !str_cmp(first_word, "system")) {
        const char* theme_name = skip_spaces(argument + fw_len);
        if (!theme_name[0]) {
            send_to_char(COLOR_INFO "You must provide the name of the system theme to remove." COLOR_EOL, ch);
            return;
        }

        if (IS_NPC(ch) || ch->pcdata->security < 9) {
            send_to_char(COLOR_INFO "You don't have permission to remove system themes." COLOR_EOL, ch);
            return;
        }

        if (system_color_theme_count <= 1) {
            send_to_char(COLOR_INFO "You must keep at least one system theme available." COLOR_EOL, ch);
            return;
        }

        int index = color_find_system_theme_index(theme_name, false);
        if (index < 0) {
            printf_to_char(ch, COLOR_INFO "No system theme named '%s' could be found." COLOR_EOL,
                theme_name);
            return;
        }

        char removed_name[MAX_INPUT_LENGTH];
        strncpy(removed_name, system_color_themes[index]->name, sizeof(removed_name));
        removed_name[sizeof(removed_name) - 1] = '\0';

        if (!color_remove_system_theme(index)) {
            send_to_char(COLOR_INFO "Unable to remove that system theme." COLOR_EOL, ch);
            return;
        }

        printf_to_char(ch, COLOR_INFO "System theme '%s' removed. Use THEME SAVE SYSTEM to persist." COLOR_EOL,
            removed_name);
        return;
    }

    ColorTheme* theme = lookup_local_color_theme(ch, argument);

    if (!theme) {
        send_to_char(COLOR_INFO "You do not have a color theme by that name." COLOR_EOL, ch);
        return;
    }

    if (str_cmp(argument, theme->name)) {
        // Lookup function uses str_prefix. Deleting stuff should be deliberate,
        // so force them to use the whole name.
        send_to_char(COLOR_INFO "To delete a color theme, you must use its full "
            "name." COLOR_EOL, ch);
        return;
    }

    if (!str_cmp(theme->name, ch->pcdata->current_theme->name)) {
        send_to_char(COLOR_INFO "You are currently using that color theme. Use " COLOR_ALT_TEXT_1 "THEME "
            "SELECT" COLOR_INFO " to switch themes before removing this one." COLOR_EOL, ch);
        return;
    }

    for (int i = 0; i < MAX_THEMES; ++i) {
        if (ch->pcdata->color_themes[i] == theme) {
            ch->pcdata->color_themes[i] = NULL;
            break;
        }
    }

    free_color_theme(theme);
    
    send_to_char(COLOR_INFO "Color theme removed." COLOR_EOL, ch);
    return;
}

static void do_theme_save(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH] = { 0 };
    argument = one_argument(argument, arg);
    if (arg[0] && !str_prefix(arg, "system")) {
        if (IS_NPC(ch) || ch->pcdata->security < 9) {
            send_to_char(COLOR_INFO "You don't have permission to do that." COLOR_EOL, ch);
            return;
        }

        PersistResult res = theme_persist_save(NULL);
        if (!persist_succeeded(res)) {
            printf_to_char(ch, COLOR_INFO "Saving system themes failed (%s)." COLOR_EOL,
                res.message ? res.message : "unknown error");
            return;
        }

        printf_to_char(ch, COLOR_INFO "System themes saved to '%s'." COLOR_EOL,
            cfg_get_themes_file());
        return;
    }

    theme_save_personal(ch);
}

static void do_theme_select(Mobile* ch, char* argument)
{
    static const char* help =
        COLOR_INFO
        "USAGE: " COLOR_ALT_TEXT_1 "THEME SELECT (@<player>) <name>" COLOR_INFO "\n\r"
        "\n\r"
        "For a list of themes to select from, type 'THEME LIST'." COLOR_EOL;

    if (!argument || !argument[0]) {
        send_to_char(help, ch);
        return;
    }

    if (ch->pcdata->current_theme->is_changed) {
        send_to_char(theme_change_warning, ch);
        return;
    }


    ColorTheme* theme = lookup_local_color_theme(ch, argument);

    if (!theme)
        theme = lookup_system_color_theme(ch, argument);

    if (!theme) {
        theme = lookup_remote_color_theme(ch, argument);

        if (!theme) {
            printf_to_char(ch, COLOR_INFO "No color theme named '%s' could be found." COLOR_EOL,
                argument);
            return;
        }

        // We have to save a copy.
        bool found = false;
        for (int i = 0; i < MAX_THEMES; ++i) {
            if (!ch->pcdata->color_themes[i]) {
                ch->pcdata->color_themes[i] = dup_color_theme(theme);
                save_char_obj(ch);
                found = true;
            }
        }

        if (!found) {
            printf_to_char(ch, COLOR_INFO "Selecting another player's theme saves a copy "
                "to your own themes. You already have too many themes to add "
                "another." COLOR_EOL);
            return;
        }
    }

    if (ch->pcdata->current_theme) {
        free_color_theme(ch->pcdata->current_theme);
    }

    ch->pcdata->current_theme = dup_color_theme(theme);
    free_string(ch->pcdata->theme_config.current_theme_name);
    ch->pcdata->theme_config.current_theme_name = 
        str_dup(ch->pcdata->current_theme->name);
    set_default_colors(ch);

    printf_to_char(ch, COLOR_INFO "Color theme is now set to '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "'." COLOR_EOL, ch->pcdata->current_theme->name);
    printf_to_char(ch, COLOR_ALT_TEXT_2 "%s" COLOR_EOL, ch->pcdata->current_theme->banner);
}

static void do_theme_set(Mobile* ch, char* argument)
{
    char opt[MAX_INPUT_LENGTH] = { 0 };
    READ_ARG(opt);

    if (!opt[0] || str_cmp(opt, "system")) {
        send_to_char(COLOR_INFO "USAGE: " COLOR_ALT_TEXT_1 "THEME SET SYSTEM DEFAULT <system theme>" COLOR_INFO COLOR_EOL, ch);
        return;
    }

    char sub[MAX_INPUT_LENGTH] = { 0 };
    READ_ARG(sub);
    if (str_prefix(sub, "default") || !argument || !argument[0]) {
        send_to_char(COLOR_INFO "USAGE: " COLOR_ALT_TEXT_1 "THEME SET SYSTEM DEFAULT <system theme>" COLOR_INFO COLOR_EOL, ch);
        return;
    }

    if (IS_NPC(ch) || ch->pcdata->security < 9) {
        send_to_char(COLOR_INFO "You don't have permission to change the default system theme." COLOR_EOL, ch);
        return;
    }

    if (!color_set_default_system_theme(argument)) {
        printf_to_char(ch, COLOR_INFO "Could not find the system theme '%s'." COLOR_EOL, argument);
        return;
    }

    const ColorTheme* def = get_default_system_color_theme();
    if (def)
        printf_to_char(ch, COLOR_INFO "System default theme is now '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "'. Use THEME SAVE SYSTEM to persist." COLOR_EOL, def->name);
    else
        send_to_char(COLOR_INFO "System default theme updated. Use THEME SAVE SYSTEM to persist." COLOR_EOL, ch);
}

void theme_show_theme(Mobile* ch, ColorTheme* theme)
{
    if (!theme) {
        send_to_char(COLOR_INFO "No color theme is available to display." COLOR_EOL, ch);
        return;
    }

    bool xterm = ch->pcdata->theme_config.xterm;

    char out[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    char bg_code[50];
    char color_ref[50];
    char mode[50] = { 0 };

    switch (theme->mode) {
    case COLOR_MODE_16:
        sprintf(mode, "(ANSI) ");
        break;
    case COLOR_MODE_256:
        sprintf(mode, "(256 Color)");
        break;
    case COLOR_MODE_RGB:
        sprintf(mode, "(24-Bit) ");
        break;
    case COLOR_MODE_PAL_IDX:
        sprintf(mode, "(error) ");
    }

    // Cache codes
    sprintf(bg_code, "%s", BGCOLOR2STR(theme, xterm));

    // First line, blank BG color
    sprintf(out, "\n\r%s" COLOR_ALT_TEXT_2 "                                   00  02  04  06  08  10  12  14      " COLOR_EOL, bg_code);

    // Second line, name and first row of color boxes
    sprintf(buf, "%s" COLOR_TEXT "    %-30s", bg_code, theme->name);
    strcat(out, buf);
    for (int i = 0; i < PALETTE_SIZE; i += 2) {
        if (i <= theme->palette_max)
            sprintf(buf, "%s    ", bg_color_to_str(theme, &theme->palette[i], xterm));
        else
            sprintf(buf, "%s    ", bg_code);
        strcat(out, buf);
    }
    sprintf(buf, "%s    " COLOR_EOL, bg_code);
    strcat(out, buf);

    // Third line, mode type and second row of color boxes
    sprintf(buf, "%s" COLOR_ALT_TEXT_1 "    %-20s          ", bg_code, mode);
    strcat(out, buf);
    for (int i = 1; i < PALETTE_SIZE; i += 2) {
        if (i <= theme->palette_max)
            sprintf(buf, "%s    ", bg_color_to_str(theme, &theme->palette[i], xterm));
        else
            sprintf(buf, "%s    ", bg_code);
        strcat(out, buf);
    }
    sprintf(buf, "%s    " COLOR_EOL, bg_code);
    strcat(out, buf);

    // Fourth line, blank BG color
    sprintf(buf, "%s" COLOR_ALT_TEXT_2 "                                   01  03  05  07  09  11  13  15      " COLOR_CLEAR "\n\r\n\r", bg_code);
    strcat(out, buf);

    int row = 0;
    strcat(out, COLOR_TITLE "Palette Swatches:" COLOR_CLEAR " (edit with " COLOR_ALT_TEXT_1 "palette" COLOR_CLEAR " in THEME EDIT)\n\r");
    for (int i = 0; i < theme->palette_max; ++i) {
        Color* color = &theme->palette[i];
        switch (color->mode) {
        case COLOR_MODE_16:
            sprintf(color_ref, "%d", color->code[0] * 8 + color->code[1]);
            break;
        case COLOR_MODE_256:
            sprintf(color_ref, "%d", color->code[0]);
            break;
        case COLOR_MODE_RGB:
            sprintf(color_ref, "#%02X%02X%02X;",
                color->code[0], color->code[1], color->code[2]);
            break;
        case COLOR_MODE_PAL_IDX:
            sprintf(color_ref, "(error)");
            break;
        }

        if (i != 1)     // By convention, 0x01 is the background-ish color.
            sprintf(buf, "    %s%02d %-15s " COLOR_CLEAR , color_to_str(theme, color, xterm),
                i, color_ref);
        else {
            sprintf(buf, "    %s%s%02d %-15s " COLOR_CLEAR ,
                bg_color_to_str(theme, &theme->palette[0], xterm),
                color_to_str(theme, &theme->palette[1], xterm),
                i, color_ref);
        }

        if (++row == 3) {
            row = 0;
            strcat(buf, "\n\r");
        }
        else
            strcat(buf, "    ");
        strcat(out, buf);
    }
    strcat(out, "\n\r\n\r");

    row = 0;
    strcat(out, COLOR_TITLE "Channel Assignments:" COLOR_CLEAR " (edit with " COLOR_ALT_TEXT_1 "channel" COLOR_CLEAR " in THEME EDIT)\n\r");
    for (int i = 0; i < COLOR_SLOT_COUNT; ++i) {
        Color* color = &theme->channels[i];
        switch (color->mode) {
        case COLOR_MODE_16:
            sprintf(color_ref, "%d", color->code[0] * 8 + color->code[1]);
            break;
        case COLOR_MODE_256:
            sprintf(color_ref, "%d", color->code[0]);
            break;
        case COLOR_MODE_RGB:
            sprintf(color_ref, "#%02X%02X%02X;", 
                color->code[0], color->code[1], color->code[2]);
            break;
        case COLOR_MODE_PAL_IDX:
            sprintf(color_ref, "PALETTE %d", color->code[0]);
            break;
        default:
            sprintf(color_ref, "(error)");
            break;
        }
        const ColorChannelEntry* channel = &color_slot_entries[i];

        if (i != SLOT_BACKGROUND)
            sprintf(buf, "    " COLOR_ESC_STR "%c" COLOR_ESC_STR COLOR_ESC_STR 
                "%c %-15s %-15s " COLOR_CLEAR, channel->code, channel->code,
                channel->name, color_ref);
        else {
            sprintf(buf, "    %s%s" COLOR_ESC_STR COLOR_ESC_STR 
                "%c %-15s %-15s " COLOR_CLEAR,
                bg_color_to_str(theme, &theme->channels[SLOT_TEXT], xterm),
                color_to_str(theme, &theme->channels[SLOT_BACKGROUND], xterm),
                channel->code, channel->name, color_ref);
        }

        if (++row == 2) {
            row = 0;
            strcat(buf, "\n\r");
        }
        else
            strcat(buf, "    ");
        strcat(out, buf);
    }
    strcat(out, "\n\r");

    send_to_char(out, ch);
}

static void do_theme_show(Mobile* ch, char* argument)
{
    ColorTheme* theme = ch->pcdata->current_theme;

    if (argument[0]) {
        theme = lookup_local_color_theme(ch, argument);

        if (!theme)
            theme = lookup_system_color_theme(ch, argument);

        if (!theme)
            theme = lookup_remote_color_theme(ch, argument);

        if (!theme) {
            printf_to_char(ch, COLOR_INFO "No color theme named '%s' could be found." COLOR_EOL,
                argument);
            return;
        }
    }

    if (!theme) {
        send_to_char(COLOR_INFO "You do not have a color theme active. You can use "
            COLOR_ALT_TEXT_1 "THEME SELECT" COLOR_INFO " to choose one, or " COLOR_ALT_TEXT_1 "THEME CREATE" COLOR_INFO " to make a new "
            "one." COLOR_EOL, ch);
        return;
    }

    theme_show_theme(ch, theme);
}

static void do_theme_edit(Mobile* ch, char* argument)
{
    static const char* usage = COLOR_INFO "USAGE: THEME EDIT or THEME EDIT SYSTEM <system theme>" COLOR_EOL;

    if (IS_NPC(ch))
        return;

    if (ch->desc == NULL) {
        send_to_char(COLOR_INFO "You cannot edit themes without an active descriptor." COLOR_EOL, ch);
        return;
    }

    argument = skip_spaces(argument);

    if (argument && argument[0]) {
        char first_word[MAX_INPUT_LENGTH] = { 0 };
        size_t fw_len = 0;
        while (argument[fw_len] && !isspace((unsigned char)argument[fw_len])
            && fw_len < MAX_INPUT_LENGTH - 1) {
            first_word[fw_len] = argument[fw_len];
            ++fw_len;
        }
        first_word[fw_len] = '\0';

        if (!str_cmp(first_word, "system")) {
            const char* name = skip_spaces(argument + fw_len);
            if (!name[0]) {
                send_to_char(usage, ch);
                return;
            }

            if (IS_NPC(ch) || ch->pcdata->security < 9) {
                send_to_char(COLOR_INFO "You don't have permission to edit system themes." COLOR_EOL, ch);
                return;
            }

            ColorTheme* theme = color_find_system_theme(name, true);
            if (!theme) {
                printf_to_char(ch, COLOR_INFO "No system theme named '%s' could be found." COLOR_EOL,
                    name);
                return;
            }

            set_editor(ch->desc, ED_THEME, (uintptr_t)theme);
            printf_to_char(ch, COLOR_INFO "Editing system theme '%s'. Use SAVE to write it to disk." COLOR_EOL,
                theme->name);
            theme_show_theme(ch, theme);
            return;
        }

        send_to_char(usage, ch);
        return;
    }

    if (ch->pcdata->current_theme == NULL)
        set_default_theme(ch);

    if (ch->pcdata->current_theme == NULL) {
        send_to_char(COLOR_INFO "You do not have a color theme active to edit." COLOR_EOL, ch);
        return;
    }

    set_editor(ch->desc, ED_THEME, (uintptr_t)ch->pcdata->current_theme);
    send_to_char(COLOR_INFO "Theme editor activated. Type 'commands' for options." COLOR_EOL, ch);
    theme_show_theme(ch, ch->pcdata->current_theme);
}

void do_theme(Mobile* ch, char* argument)
{
    static const char* help =
        COLOR_INFO
        "USAGE: " COLOR_ALT_TEXT_1 "THEME CONFIG\n\r"
        "       THEME CREATE (ANSI|256|RGB) <name>\n\r"
        "       THEME EDIT (SYSTEM <name>)\n\r"
        "       THEME LIST (ALL|SYSTEM|PUBLIC|PRIVATE)\n\r"
        "       THEME PREVIEW (<name>)\n\r"
        "       THEME REMOVE <name>\n\r"
        "       THEME SAVE (SYSTEM)\n\r"
        "       THEME SELECT <name>\n\r"
        "       THEME SET SYSTEM DEFAULT <system theme>\n\r"
        "       THEME SHOW (<name>)" COLOR_INFO "\n\r"
        "\n\r"
        "Type '" COLOR_ALT_TEXT_1 "THEME <option>" COLOR_INFO "' for more information."
        "\n\r"
        COLOR_INFO "Sec 9 immortals can prefix theme names with 'system' in CREATE,"
        " REMOVE, and EDIT to manage system themes." COLOR_EOL;

    char cmd[MAX_INPUT_LENGTH] = { 0 };

    if (IS_NPC(ch)) {
        return;
    }

    if (!IS_SET(ch->act_flags, PLR_COLOUR)) {
        send_to_char_bw("You have colors disabled. Use the COLOR command "
            "to enabled them before setting color themes.\n\r", ch);
        return;
    }

    READ_ARG(cmd);

    if (!str_prefix(cmd, "help")) {
        send_to_char(help, ch);
    }
    else if (!str_prefix(cmd, "config")) {
        do_theme_config(ch, argument);
    }
    else if (!str_prefix(cmd, "create")) {
        do_theme_create(ch, argument);
    }
    else if (!str_prefix(cmd, "list")) {
        do_theme_list(ch, argument);
    }
    else if (!str_prefix(cmd, "edit")) {
        do_theme_edit(ch, argument);
    }
    else if (!str_prefix(cmd, "preview")) {
        do_theme_preview(ch, argument);
    }
    else if (!str_cmp(cmd, "remove")) {
        do_theme_remove(ch, argument);
    }
    else if (!str_prefix(cmd, "save")) {
        do_theme_save(ch, argument);
    }
    else if (!str_prefix(cmd, "select")) {
        do_theme_select(ch, argument);
    }
    else if (!str_prefix(cmd, "set")) {
        do_theme_set(ch, argument);
    }
    else if (!str_prefix(cmd, "show")) {
        do_theme_show(ch, argument);
    }
    else
        send_to_char(help, ch);
}

ColorTheme* dup_color_theme(const ColorTheme* theme)
{
    ColorTheme* copy = new_color_theme();
    memcpy(copy, theme, sizeof(ColorTheme));
    copy->name = str_dup(theme->name);
    copy->banner = str_dup(theme->banner);

    if (theme->type != COLOR_THEME_TYPE_SYSTEM)
        copy->is_public = theme->is_public;

    for (int i = 0; i < theme->palette_max; ++i) {
        if (theme->palette[i].cache != NULL && theme->palette[i].cache[0])
            copy->palette[i].cache = str_dup(theme->palette[i].cache);
        if (theme->palette[i].xterm != NULL && theme->palette[i].xterm[0])
            copy->palette[i].xterm = str_dup(theme->palette[i].xterm);
    }

    for (int i = 0; i < COLOR_SLOT_COUNT; ++i) {
        if (theme->channels[i].cache != NULL && theme->channels[i].cache[0])
            copy->channels[i].cache = str_dup(theme->channels[i].cache);
        if (theme->channels[i].xterm != NULL && theme->channels[i].xterm[0])
            copy->channels[i].xterm = str_dup(theme->channels[i].xterm);
    }

    // Keep track if it's am unchanged copy of a system theme. Don't save it 
    // if it is.
    if (theme->type == COLOR_THEME_TYPE_SYSTEM)
        copy->type = COLOR_THEME_TYPE_SYSTEM_COPY;

    return copy;
}

void free_color_theme(ColorTheme* theme)
{
    if (theme == NULL)
        return;

    if (theme->type == COLOR_THEME_TYPE_SYSTEM)
        return;

    clear_cached_codes(theme);

    free_string(theme->name);
    free_string(theme->banner);
    free_mem(theme, sizeof(ColorTheme));
}

ColorMode lookup_color_mode(const char* arg)
{
    if (!str_prefix(arg, "ansi"))
        return COLOR_MODE_16;
    else if (!str_prefix(arg, "256"))
        return COLOR_MODE_256;
    else if (!str_prefix(arg, "rgb"))
        return COLOR_MODE_RGB;
    else if (!str_prefix(arg, "palette"))
        return COLOR_MODE_PAL_IDX;
    else
        return -1;
}

ColorTheme* lookup_color_theme(Mobile* ch, char* arg)
{
    ColorTheme* theme;

    if ((theme = lookup_remote_color_theme(ch, arg)) != NULL ||
        (theme = lookup_local_color_theme(ch, arg)) != NULL ||
        (theme = lookup_system_color_theme(ch, arg)) != NULL)
        return theme;

    return NULL;
}

ColorTheme* lookup_system_color_theme(Mobile* ch, char* arg)
{
    bool show_256 = !ch->pcdata->theme_config.hide_256;
    bool show_24bit = !ch->pcdata->theme_config.hide_24bit;

    for (int i = 0; i < system_color_theme_count; ++i) {
        if (!system_color_themes[i])
            continue;
        if (!str_prefix(arg, system_color_themes[i]->name)
            && (show_256 || system_color_themes[i]->mode != COLOR_MODE_256)
            && (show_24bit || system_color_themes[i]->mode != COLOR_MODE_RGB))
            // Casting away const is bad. But soon I'll read it from a file
            // and it won't be const anymore, anyway.
            return (ColorTheme*)system_color_themes[i];
    }

    return NULL;
}

ColorTheme* lookup_remote_color_theme(Mobile* ch, char* arg)
{
    bool show_256 = !ch->pcdata->theme_config.hide_256;
    bool show_24bit = !ch->pcdata->theme_config.hide_24bit;

    if (arg[0] == '@') {
        ColorTheme* theme;
        char name[50] = { 0 };

        arg = one_argument(arg + 1, name);

        for (Descriptor* d = descriptor_list; d != NULL; NEXT_LINK(d)) {
            if (d->connected != CON_PLAYING || !can_see(ch, d->character))
                continue;

            Mobile* wch = (d->original != NULL) ? d->original : d->character;

            if (str_prefix(name, NAME_STR(wch)))
                continue;

            if (!can_see(ch, wch))
                continue;

            for (int i = 0; i < MAX_THEMES; ++i) {
                theme = wch->pcdata->color_themes[i];
                if (!theme || !theme->is_public)
                    continue;

                if (!str_prefix(arg, theme->name)
                    && (show_256 || theme->mode != COLOR_MODE_256)
                    && (show_24bit || theme->mode != COLOR_MODE_RGB))
                    return theme;
            }
        }

        printf_to_char(ch, COLOR_INFO "Could not find the theme '%s' or person '%s'.\n\r",
            name, arg);
    }

    return NULL;
}

ColorTheme* lookup_local_color_theme(Mobile* ch, char* arg)
{
    bool show_256 = !ch->pcdata->theme_config.hide_256;
    bool show_24bit = !ch->pcdata->theme_config.hide_24bit;

    ColorTheme** themes = ch->pcdata->color_themes;
    for (int i = 0; i < MAX_THEMES; i++) {
        if (themes[i] && !str_prefix(arg, themes[i]->name)
            && (show_256 || themes[i]->mode != COLOR_MODE_256)
            && (show_24bit || themes[i]->mode != COLOR_MODE_RGB))
            return themes[i];
    }

    return NULL;
}

ColorTheme* new_color_theme()
{
    ColorTheme* theme = (ColorTheme*)alloc_mem(sizeof(ColorTheme));
    memset(theme, 0, sizeof(ColorTheme));
    return theme;
}

void set_color_ansi(Color* color, uint8_t light, uint8_t index)
{
    color->mode = COLOR_MODE_16;
    color->code[0] = light;
    color->code[1] = index;
    color->code[2] = 0;

    free_string(color->cache);
    color->cache = NULL;
    free_string(color->xterm);
    color->xterm = NULL;
}

void set_color_256(Color* color, uint8_t index)
{
    color->mode = COLOR_MODE_256;
    color->code[0] = index;
    color->code[1] = 0;
    color->code[2] = 0;

    free_string(color->cache);
    color->cache = NULL;
    free_string(color->xterm);
    color->xterm = NULL;
}

void set_color_palette_ref(Color* color, uint8_t index)
{
    color->mode = COLOR_MODE_PAL_IDX;
    color->code[0] = index;
    color->code[1] = 0;
    color->code[2] = 0;

    free_string(color->cache);
    color->cache = NULL;
    free_string(color->xterm);
    color->xterm = NULL;
}

void set_color_rgb(Color* color, uint8_t r, uint8_t g, uint8_t b)
{
    color->mode = COLOR_MODE_RGB;
    color->code[0] = r;
    color->code[1] = g;
    color->code[2] = b;

    free_string(color->cache);
    color->cache = NULL;
    free_string(color->xterm);
    color->xterm = NULL;
}

void set_default_theme(Mobile* ch)
{
    if (IS_NPC(ch))
        return;

    if (ch->pcdata == NULL)
        return;

    if (ch->pcdata->current_theme) {
        if (ch->pcdata->current_theme->is_changed) {
            send_to_char(theme_change_warning, ch);
            return;
        }
        free_color_theme(ch->pcdata->current_theme);
    }

    const ColorTheme* default_theme = get_default_system_color_theme();
    if (!default_theme) {
        bugf("set_default_theme: no system color themes are available.");
        return;
    }
    ch->pcdata->current_theme = dup_color_theme(default_theme);
    free_string(ch->pcdata->theme_config.current_theme_name);
    ch->pcdata->theme_config.current_theme_name = str_dup(ch->pcdata->current_theme->name);

    return;
}

void set_default_colors(Mobile* ch)
{
    char out[MAX_INPUT_LENGTH];
    bool xterm = ch->pcdata->theme_config.xterm;

    if (IS_NPC(ch) || ch->pcdata == NULL)
        return;

    if (ch->pcdata->current_theme == NULL)
        set_default_theme(ch);

    if (ch->pcdata->current_theme == NULL)
        return;

    ColorTheme* theme = ch->pcdata->current_theme;

    sprintf(out, "%s%s" VT_SAVE_SGR,
        color_to_str(theme, &theme->channels[SLOT_TEXT], xterm),
        bg_color_to_str(theme, &theme->channels[SLOT_BACKGROUND], xterm)
    );

    send_to_char(out, ch);
}

////////////////////////////////////////////////////////////////////////////////
// Channel Lookups
////////////////////////////////////////////////////////////////////////////////

const ColorChannelEntry color_slot_entries[COLOR_SLOT_COUNT] = {
    { "text",			    SLOT_TEXT,	            't' },
    { "background",		    SLOT_BACKGROUND,	    'X' },
    { "title",              SLOT_TITLE,             'T' },
    { "alt_text",           SLOT_ALT_TEXT_1,        '*' },
    { "alt_text2",          SLOT_ALT_TEXT_2,        '_' },
    { "decor1",             SLOT_DECOR_1,           '|' },
    { "decor2",             SLOT_DECOR_2,           '=' },
    { "decor3",             SLOT_DECOR_3,           '\\'},
    { "auction",			SLOT_AUCTION,	        'a' },
    { "auction_text",		SLOT_AUCTION_TEXT,	    'A' },
    { "gossip",			    SLOT_GOSSIP,	        'd' },
    { "gossip_text",		SLOT_GOSSIP_TEXT,	    '9' },
    { "music",			    SLOT_MUSIC,	            'e' },
    { "music_text",			SLOT_MUSIC_TEXT,	    'E' },
    { "question",			SLOT_QUESTION,	        'q' },
    { "question_text",		SLOT_QUESTION_TEXT,	    'Q' },
    { "answer",			    SLOT_ANSWER,	        'f' },
    { "answer_text",		SLOT_ANSWER_TEXT,	    'F' },
    { "quote",			    SLOT_QUOTE,	            'h' },
    { "quote_text",			SLOT_QUOTE_TEXT,	    'H' },
    { "immtalk_text",		SLOT_IMMTALK_TEXT,	    'i' },
    { "immtalk_type",		SLOT_IMMTALK_TYPE,	    'I' },
    { "info",			    SLOT_INFO,	            'j' },
    { "say",			    SLOT_SAY,	            '6' },
    { "say_text",			SLOT_SAY_TEXT,	        '7' },
    { "tell",			    SLOT_TELL,	            'k' },
    { "tell_text",			SLOT_TELL_TEXT,	        'K' },
    { "reply",			    SLOT_REPLY,	            'l' },
    { "reply_text",			SLOT_REPLY_TEXT,	    'L' },
    { "gtell_text",			SLOT_GTELL_TEXT,	    'n' },
    { "gtell_type",			SLOT_GTELL_TYPE,	    'N' },
    { "wiznet",			    SLOT_WIZNET,	        'Z' },
    { "room_title",			SLOT_ROOM_TITLE,	    's' },
    { "room_text",			SLOT_ROOM_TEXT,	        'S' },
    { "room_exits",			SLOT_ROOM_EXITS,	    'o' },
    { "room_things",		SLOT_ROOM_THINGS,       'O' },
    { "prompt",			    SLOT_PROMPT,	        'p' },
    { "fight_death",		SLOT_FIGHT_DEATH,	    '1' },
    { "fight_yhit",			SLOT_FIGHT_YHIT,	    '2' },
    { "fight_ohit",			SLOT_FIGHT_OHIT,	    '3' },
    { "fight_thit",			SLOT_FIGHT_THIT,	    '4' },
    { "fight_skill",		SLOT_FIGHT_SKILL,       '5' },

    { "lox_comment",        SLOT_LOX_COMMENT,       '`' },
    { "lox_string",         SLOT_LOX_STRING,        '!' },
    { "lox_operator",       SLOT_LOX_OPERATOR,      '&' },
    { "lox_literal",        SLOT_LOX_LITERAL,       ':' },
    { "lox_keyword",        SLOT_LOX_KEYWORD,       '@' },
};

////////////////////////////////////////////////////////////////////////////////
// Theme Helpers
////////////////////////////////////////////////////////////////////////////////

#define THEME_ENTRY_ANSI(l, c, s) \
    { .mode = COLOR_MODE_16, .code = { l, c, 0 }, .cache = s, .xterm = NULL }

#define ANSI_WHITE                  THEME_ENTRY_ANSI(NORMAL, WHITE,     "\033[37m")
#define ANSI_BLACK                  THEME_ENTRY_ANSI(NORMAL, BLACK,     "\033[30m")
#define ANSI_RED                    THEME_ENTRY_ANSI(NORMAL, RED,       "\033[31m")
#define ANSI_GREEN                  THEME_ENTRY_ANSI(NORMAL, GREEN,     "\033[32m")
#define ANSI_YELLOW                 THEME_ENTRY_ANSI(NORMAL, YELLOW,    "\033[33m")
#define ANSI_BLUE                   THEME_ENTRY_ANSI(NORMAL, BLUE,      "\033[34m")
#define ANSI_MAGENTA                THEME_ENTRY_ANSI(NORMAL, MAGENTA,   "\033[35m")
#define ANSI_CYAN                   THEME_ENTRY_ANSI(NORMAL, CYAN,      "\033[36m")
#define ANSI_D_GRAY                 THEME_ENTRY_ANSI(BRIGHT, BLACK,     "\033[90m")
#define ANSI_B_RED                  THEME_ENTRY_ANSI(BRIGHT, RED,       "\033[91m")
#define ANSI_B_GREEN                THEME_ENTRY_ANSI(BRIGHT, GREEN,     "\033[92m")
#define ANSI_B_YELLOW               THEME_ENTRY_ANSI(BRIGHT, YELLOW,    "\033[93m")
#define ANSI_B_BLUE                 THEME_ENTRY_ANSI(BRIGHT, BLUE,      "\033[94m")
#define ANSI_B_MAGENTA              THEME_ENTRY_ANSI(BRIGHT, MAGENTA,   "\033[95m")
#define ANSI_B_CYAN                 THEME_ENTRY_ANSI(BRIGHT, CYAN,      "\033[96m")
#define ANSI_B_WHITE                THEME_ENTRY_ANSI(BRIGHT, WHITE,     "\033[97m")
#define ANSI_MAX                    15

const AnsiPaletteEntry ansi_palette[ANSI_MAX] = {
    { "red",        NORMAL, RED,        ANSI_RED        },
    { "hi-red",     BRIGHT, RED,        ANSI_B_RED      },
    { "green",      NORMAL, GREEN,      ANSI_GREEN      },
    { "hi-green",   BRIGHT, GREEN,      ANSI_B_GREEN    },
    { "yellow",     NORMAL, YELLOW,     ANSI_YELLOW     },
    { "hi-yellow",  BRIGHT, YELLOW,     ANSI_B_YELLOW   },
    { "blue",       NORMAL, BLUE,       ANSI_BLUE       },
    { "hi-blue",    BRIGHT, BLUE,       ANSI_B_BLUE     },
    { "magenta",    NORMAL, MAGENTA,    ANSI_MAGENTA    },
    { "hi-magenta", BRIGHT, MAGENTA,    ANSI_B_MAGENTA  },
    { "cyan",       NORMAL, CYAN,       ANSI_CYAN       },
    { "hi-cyan",    BRIGHT, CYAN,       ANSI_B_CYAN     },
    { "white",      NORMAL, WHITE,      ANSI_WHITE      },
    { "hi-white",   BRIGHT, WHITE,      ANSI_B_WHITE    },
    { "gray",       BRIGHT, BLACK,      ANSI_D_GRAY     },
};

const ColorTheme** system_color_themes = NULL;
int system_color_theme_count = 0;

static ColorTheme** loaded_system_color_themes = NULL;
static int loaded_system_color_theme_count = 0;

static void destroy_system_theme(ColorTheme* theme)
{
    if (!theme)
        return;

    clear_cached_codes(theme);
    free_string(theme->name);
    free_string(theme->banner);
    free_mem(theme, sizeof(ColorTheme));
}

static bool ensure_system_theme_data(void)
{
    if (!system_color_themes || system_color_theme_count <= 0)
        load_system_color_themes();
    return system_color_themes && system_color_theme_count > 0;
}

static int color_find_system_theme_index(const char* name, bool allow_prefix)
{
    if (!name || !name[0])
        return -1;

    if (!ensure_system_theme_data())
        return -1;

    for (int i = 0; i < system_color_theme_count; ++i) {
        const ColorTheme* theme = system_color_themes[i];
        if (!theme)
            continue;
        if (allow_prefix) {
            if (!str_prefix(name, theme->name))
                return i;
        }
        else if (!str_cmp(name, theme->name))
            return i;
    }

    return -1;
}

static ColorTheme* color_find_system_theme(const char* name, bool allow_prefix)
{
    int index = color_find_system_theme_index(name, allow_prefix);
    if (index < 0)
        return NULL;
    return (ColorTheme*)system_color_themes[index];
}

static bool color_append_system_theme(ColorTheme* theme)
{
    if (!theme)
        return false;

    size_t new_count = (size_t)loaded_system_color_theme_count + 1;
    ColorTheme** list = NULL;

    if (!loaded_system_color_themes)
        list = (ColorTheme**)calloc(new_count, sizeof(ColorTheme*));
    else
        list = (ColorTheme**)realloc(loaded_system_color_themes, new_count * sizeof(ColorTheme*));

    if (!list)
        return false;

    loaded_system_color_themes = list;
    loaded_system_color_themes[loaded_system_color_theme_count++] = theme;
    system_color_themes = (const ColorTheme**)loaded_system_color_themes;
    system_color_theme_count = loaded_system_color_theme_count;
    return true;
}

static bool color_remove_system_theme(int index)
{
    if (index < 0 || index >= loaded_system_color_theme_count)
        return false;
    if (loaded_system_color_theme_count <= 1)
        return false;

    destroy_system_theme(loaded_system_color_themes[index]);

    for (int i = index; i < loaded_system_color_theme_count - 1; ++i)
        loaded_system_color_themes[i] = loaded_system_color_themes[i + 1];

    --loaded_system_color_theme_count;

    if (loaded_system_color_theme_count == 0) {
        free(loaded_system_color_themes);
        loaded_system_color_themes = NULL;
        system_color_themes = NULL;
        system_color_theme_count = 0;
        return true;
    }

    ColorTheme** list = (ColorTheme**)realloc(loaded_system_color_themes,
        sizeof(ColorTheme*) * (size_t)loaded_system_color_theme_count);
    if (list)
        loaded_system_color_themes = list;

    system_color_themes = (const ColorTheme**)loaded_system_color_themes;
    system_color_theme_count = loaded_system_color_theme_count;
    return true;
}

static void free_loaded_system_themes(void)
{
    if (!loaded_system_color_themes)
        return;

    for (int i = 0; i < loaded_system_color_theme_count; ++i) {
        destroy_system_theme(loaded_system_color_themes[i]);
    }

    free(loaded_system_color_themes);
    loaded_system_color_themes = NULL;
    loaded_system_color_theme_count = 0;
    system_color_themes = NULL;
    system_color_theme_count = 0;
}

bool color_register_system_themes(ColorTheme** themes, int count)
{
    if (!themes || count <= 0)
        return false;

    free_loaded_system_themes();
    loaded_system_color_themes = themes;
    loaded_system_color_theme_count = count;
    system_color_themes = (const ColorTheme**)loaded_system_color_themes;
    system_color_theme_count = count;
    return true;
}

bool color_set_default_system_theme(const char* name)
{
    if (!name || !name[0])
        return false;

    if (!system_color_themes || system_color_theme_count == 0)
        load_system_color_themes();

    if (!system_color_themes || system_color_theme_count == 0)
        return false;

    if (!loaded_system_color_themes)
        return false;

    int target = -1;
    for (int i = 0; i < system_color_theme_count; ++i) {
        if (system_color_themes[i] && !str_cmp(system_color_themes[i]->name, name)) {
            target = i;
            break;
        }
    }

    if (target < 0)
        return false;
    if (target == 0)
        return true;

    ColorTheme* tmp = loaded_system_color_themes[0];
    loaded_system_color_themes[0] = loaded_system_color_themes[target];
    loaded_system_color_themes[target] = tmp;
    return true;
}

const ColorTheme* get_default_system_color_theme()
{
    if (!system_color_themes || system_color_theme_count == 0)
        load_system_color_themes();

    if (!system_color_themes || system_color_theme_count == 0)
        return NULL;

    return system_color_themes[0];
}

void load_system_color_themes()
{
    PersistResult res = theme_persist_load(NULL);
    if (!persist_succeeded(res)) {
        const char* file = cfg_get_themes_file();
        bugf("load_system_color_themes: failed to load %s (%s).",
            file, res.message ? res.message : "unknown error");
        exit(1);
    }

    if (!system_color_themes || system_color_theme_count <= 0) {
        bugf("load_system_color_themes: %s did not define any themes.", cfg_get_themes_file());
        exit(1);
    }

    printf_log("System color themes loaded (%d themes).\n", system_color_theme_count);
}
