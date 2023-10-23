////////////////////////////////////////////////////////////////////////////////
// colors.c
// Functions to handle VT102 SGR colors
////////////////////////////////////////////////////////////////////////////////

#include "color.h"

#include "comm.h"
#include "db.h"
#include "digest.h"
#include "handler.h"
#include "save.h"
#include "vt.h"

#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/mobile_data.h"
#include "data/player.h"

#include <stdint.h>
#include <string.h>

#define COLOR2STR(t, s, x)  color_to_str((ColorTheme*)t, (Color*)&t->channels[s], x)
#define BGCOLOR2STR(t, x)   bg_color_to_str((ColorTheme*)t, (Color*)&t->channels[SLOT_BACKGROUND], x)

#define IS_HEX(c)   ((c >= '0' && c <= '9') \
        || (c >= 'A' && c <='F')            \
        || (c >= 'a' && c <= 'f'))

static const char* theme_change_warning = "{jYou have have made changes to your "
    "active theme that must either be saved with {*THEME SAVE{j or discarded "
    "with {*THEME DISCARD{x.\n\r";

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
    addf_buf(out, "%s                                                                      {x\n\r", bg_code);

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
    addf_buf(out, "%s    {x\n\r", bg_code);

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
    addf_buf(out, "%s    {x\n\r", bg_code);

    // Fourth line, blank BG color
    addf_buf(out, "%s                                                                      {x\n\r\n\r", bg_code);

    send_to_char(out->string, ch);

    free_buf(out);
}

static inline void send_256_color_list(Mobile* ch)
{
    int i;
    int text;
    send_to_char("{jYou can choose from the following:{x\n\r", ch);
    for (int row = 0; row < 16; ++row)
        for (int col = 0; col < 16; ++col) {
            i = row * 16 + col;
            text = 0;
            if (i == 0 || i == 8 || (i >= 16 && i < 22) || (i >= 232 && i < 240))
                text = 15;
            printf_to_char(ch,
                "\033[" SGR_MODE_BG ";" SGR_BITS_8 ";%dm"
                "\033[" SGR_MODE_FG ";" SGR_BITS_8 ";%dm"
                " %-3d{x", i, text, i);
            if (col == 15)
                send_to_char("\n\r", ch);
        }
    send_to_char("\n\r", ch);
}

static inline void send_ansi_color_list(Mobile* ch)
{
    send_to_char("{jYou can choose from the following:{x\n\r", ch);
    for (int i = 0; i < 15; ++i) {
        printf_to_char(ch, "    %s%-10s{x", ansi_palette[i].color.cache,
            ansi_palette[i].name);
        if ((i + 1) % 3 == 0)
            send_to_char("\n\r", ch);
    }
    send_to_char("\n\r", ch);
}

static inline bool lookup_color(char* argument, Color* color, Mobile* ch)
{
    if (argument == NULL || !argument[0]) {
        send_to_char("{jYou must choose a color.{x\n\r", ch);
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
                send_to_char("{jRGB color codes must be hexadecimal values of "
                    "the format {*#RRBBGG;{j.{x\n\r", ch);
                return false;
            }
        }

        if (color_arg[7] != ';' || color_arg[8] != '\0') {
            send_to_char("{jRGB color codes must be hexadecimal values of "
                "the format {*#RRGGBB;{j.{x\n\r", ch);
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
            send_to_char("{jA palette space must be specified by number "
                "from 0 to 15.{x\n\r", ch);
            return false;
        }

        idx = atoi(pal_arg);

        if (idx < 0 || idx > pal_max) {
            printf_to_char(ch, "{jA palette space must be specified by number "
                "in range of the palette (0 to %d).{x\n\r", pal_max);
            return false;
        }

        set_color_palette_ref(color, (uint8_t)idx);
    }
    else {
        if (ch->pcdata->current_theme->mode == COLOR_MODE_RGB) {
            send_to_char("{jRGB color codes must be hexadecimal values of "
                "the format {*#RRBBGG;{j.{x or a palette reference.\n\r", ch);
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
            printf_to_char(ch, "{jUnknown ANSI color name '{*%s{j'.\n\r",
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

static void do_theme_channel(Mobile* ch, char* argument)
{
    static const char* help =
        "USAGE: {*THEME CHANNEL <channel> <color>{x\n\r"
        "\n\r"
        "{*index{x - Palette slot in the current theme to replace. Use THEME SHOW or \n\r"
        "        THEME PREVIEW for a list of current palette assignments.\n\r\n\r"
        "{*color{x - For ANSI color themes, {*<color>{x is a numeric value representing the \n\r"
        "        standard ANSI color palette index, {_or{x an ANSI color name. For 256-color\n\r"
        "        indexed themes, this is a numeric value from 0 to 256.\n\r\n\r"
        "        RGB themes (24-bit) use colors in the form {*#RRGGBB;{x.\n\r\n\r"
        "        You can also refer directly to a palette color with {*PALETTE <index>{x\n\r"
        "        or {*PAL <index>{x.\n\r";

    char chan_arg[MAX_INPUT_LENGTH];
    int chan = -1;
    ColorTheme* theme = ch->pcdata->current_theme;

    READ_ARG(chan_arg);
    if (!chan_arg[0]) {
        send_to_char(help, ch);
        return;
    }

    LOOKUP_COLOR_SLOT_NAME(chan, chan_arg);
    if (chan < 0) {
        printf_to_char(ch, "{j'%s' is not a valid channel name. Type '{*THEME SHOW{x for a list "
            "of color channels.\n\r", chan_arg);
        return;
    }

    bool color_is_num = is_number(argument);
    int idx = -1;

    if (color_is_num)
        idx = atoi(argument);

    Color color = { 0 };
    if (theme->mode == COLOR_MODE_16) {
        if (!color_is_num) {
            if (!lookup_color(argument, &color, ch))
                return;
        }
        else if (idx < 0 || idx > 16) {
            printf_to_char(ch, "{jYou much choose a color index from 0 to 15.{x\n\r");
            return;
        }
        else
            set_color_ansi(&color, (uint8_t)idx / 8, (uint8_t)idx % 8);
    }
    else if (theme->mode == COLOR_MODE_256) {
        if (!color_is_num) {
            if (!lookup_color(argument, &color, ch))
                return;
        }
        else if (idx < 0 || idx > 255) {
            printf_to_char(ch, "{jYou much choose a color index from 0 to 255.{x\n\r");
            send_256_color_list(ch);
            return;
        }
        else
            set_color_256(&color, (uint8_t)idx);
    }
    else if (theme->mode == COLOR_MODE_RGB) {
        if (!lookup_color(argument, &color, ch))
            return;
    }
    else {
        printf_to_char(ch, "{jTheme palettes cannot refer to themselves. Choose a color.{x\n\r.");
    }

    free_string(theme->channels[chan].cache);
    theme->channels[chan].cache = NULL;
    free_string(theme->channels[chan].xterm);
    theme->channels[chan].xterm = NULL;

    theme->channels[chan] = color;
    theme->is_changed = true;
}

static void clear_cached_codes(ColorTheme* theme)
{
    for (int i = 0; i < theme->palette_max; ++i) {
        free_string(theme->palette[i].cache);
        theme->palette[i].cache = NULL;
        free_string(theme->palette[i].xterm);
        theme->palette[i].xterm = NULL;
    }

    for (int i = 0; i < SLOT_MAX; ++i) {
        free_string(theme->channels[i].cache);
        theme->channels[i].cache = NULL;
        free_string(theme->channels[i].xterm);
        theme->channels[i].xterm = NULL;
    }
}

static void do_theme_config(Mobile* ch, char* argument)
{
    static const char* help =
        "USAGE: {*THEME CONFIG (ENABLE|DISABLE) (256|RGB|XTERM){x\n\r"
        "\n\r"
        "Toggle certain themes, depending on your client's capabilities.\n\r"
        "Has the following arguments:\n\r"
        "       {*ENABLE    {x- Enables the given mode (on by default). \n\r"
        "       {*DISABLE   {x- Hide themes of the given mode from {*LIST{x.\n\r\n\r"
        "       {*256       {x- Show/hide 256-color extended index palettes.\n\r"
        "       {*RGB       {x- Show/hide 24-bit RGB palettes.\n\r"
        "       {*XTERM     {x- Use {_xterm{x 24-bit color codes instead of VT100. This\n\r"
        "                   is used by Linux terminals and TinTin++. If 24-bit themes\n\r"
        "                   don't appear for you, try this first.\n\r"
        "       {*RGB_HELP  {x- Show/hide long-winded message at the end of {*THEME_LIST{x.\n\r"
        "\n\r";

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
        printf_to_char(ch, "{j256-color mode is now %s.\n\r", 
            enable ? "enabled" : "disabled");
    }
    else if (!str_cmp(opt, "rgb")) {
        ch->pcdata->theme_config.hide_24bit = !enable;
        printf_to_char(ch, "{j24-bit color mode is now %s.\n\r",
            enable ? "enabled" : "disabled");
    }
    else if (!str_cmp(opt, "xterm")) {
        ch->pcdata->theme_config.xterm = enable;
        printf_to_char(ch, "{jxterm mode is now %s.\n\r",
            enable ? "enabled" : "disabled");
    }
    else if (!str_cmp(opt, "rgb_help")) {
        ch->pcdata->theme_config.hide_rgb_help = !enable;
        printf_to_char(ch, "{jRGB list help is now %s.{x\n\r",
            capitalize(opt), !enable ? "enabled" : "disabled");
        return;
    }
    else {
        send_to_char(help, ch);
    }
}

static void do_theme_create(Mobile* ch, char* argument)
{
    static const char* help =
        "USAGE: {*THEME CREATE (ANSI|256|RGB) <name>{x\n\r"
        "\n\r"
        "Creates a new theme of the requested type and sets it to the currently "
        "active theme. The default is a solid black background and soft-white "
        "foreground colors.\n\r"
        "\n\r"
        "All channel colors are assigned to the first palette index except "
        "background, which is set to the second palette index (black by "
        "default). These values can be changed.\n\r";

    char mode_arg[MAX_INPUT_LENGTH];
    ColorMode mode;

    if (ch->pcdata->current_theme->is_changed) {
        send_to_char(theme_change_warning, ch);
        return;
    }

    READ_ARG(mode_arg);

    if (!mode_arg[0] 
        || (mode = lookup_color_mode(mode_arg)) < COLOR_MODE_16
        || mode > COLOR_MODE_RGB
        || !argument[0]) {
        send_to_char(help, ch);
        return;
    }

    size_t len = strlen(argument);
    if (len < 2 || len > 12) {
        send_to_char("{jColor theme names must be between 2 and 12 characters long.{x\n\r", ch);
        return;
    }

    for (int i = 0; i < SYSTEM_COLOR_THEME_MAX; ++i) {
        if (!str_cmp(argument, system_color_themes[i]->name)) {
            send_to_char("{jYou cannot use the name of an existing system or "
                "personal theme.{x\n\r", ch);
            return;
        }
    }

    int slot = -1;
    for (int i = 0; i < MAX_THEMES; ++i) {
        if (ch->pcdata->color_themes[i] == NULL) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        send_to_char("{jYou already have the maximum number of personal color "
            "themes. You must remove one with 'THEME REMOVE' first.{x\n\r", ch);
        return;
    }

    ColorTheme* theme = new_color_theme();
    theme->name = str_dup(argument);
    theme->mode = mode;
    theme->banner = str_dup("");
    theme->palette_max = 16;
    theme->is_changed = true;
    theme->is_public = false;
    
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

    Color fg_idx = (Color){ .mode = COLOR_MODE_PAL_IDX, .code = {0, 0, 0 }, .cache = NULL, .xterm = NULL };
    Color bg_idx = (Color){ .mode = COLOR_MODE_PAL_IDX, .code = {1, 0, 0 }, .cache = NULL, .xterm = NULL };
    for (int i = 0; i < SLOT_MAX; ++i)
        theme->channels[i] = fg_idx;
    theme->channels[SLOT_BACKGROUND] = bg_idx;

    free_color_theme(ch->pcdata->current_theme);
    ch->pcdata->current_theme = theme;
    ch->pcdata->color_themes[slot] = theme;

    printf_to_char(ch, "New theme created: %s\n\r", theme->name);
}

static void do_theme_discard(Mobile* ch, char* argument)
{
    ColorTheme* current_theme = ch->pcdata->current_theme;
    char* theme_name = ch->pcdata->theme_config.current_theme_name;

    if (!current_theme->is_changed && !str_cmp(current_theme->name, theme_name)) {
        printf_to_char(ch, "{jYour current theme '%s' has no pending changes.\n\r", 
            current_theme->name);
        return;
    }

    ColorTheme* theme = lookup_color_theme(ch, theme_name);
    if (!theme) {
        printf_to_char(ch, "{jCan no longer find a theme named '%s'.\n\r",
            theme_name);
        return;
    }

    free_color_theme(current_theme);
    ch->pcdata->current_theme = dup_color_theme(theme);
}

static void do_theme_list(Mobile* ch, char* argument)
{
    static const char* help =
        "USAGE: {*THEME LIST (ALL|SYSTEM|PUBLIC|PRIVATE){x\n\r"
        "\n\r"
        "Lists the available themes. Has the following arguments:\n\r"
        "       {*ALL     {x- Show all available themes you can select from. (default option)\n\r"
        "       {*SYSTEM  {x- Only show built-in themes.\n\r"
        "       {*PUBLIC  {x- Only show themes made public by current, logged-in users.\n\r"
        "       {*PRIVATE {x- Only show your own personal themes.\n\r";

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
                    send_to_char("\n\r{TYour personal themes:\n\r", ch);
                    send_to_char("{============================================"
                        "==========================={x\n\r", ch);
                    found = true;
                }
                list_theme_entry(ch, ch->pcdata->color_themes[i], NULL, true);
            }
        }
    }

    if (system) {
        send_to_char("{TSystem themes:\n\r", ch);
        send_to_char("{========================================================"
            "==============={x\n\r", ch);
        for (int i = 0; i < SYSTEM_COLOR_THEME_MAX; ++i) {
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
                    send_to_char("{TPublic themes:\n\r", ch);
                    send_to_char("{============================================"
                        "==========================={x\n\r", ch);
                    found = true;
                }
                list_theme_entry(ch, theme, wch->name, false);
            }
        }
    }

    if (!ch->pcdata->theme_config.hide_rgb_help) {
        send_to_char(
            "{jIf you do not see any colors for 24-bit themes:{x\n\r"
            "  {**{x Check your client to make sure it supports 24-bit color.\n\r"
            "  {**{x Your client may want {_xterm{x color codes instead of VT-102 SGR commands\n\r"
            "    (TinTin++, for example). Try {*THEME CONFIG ENABLE XTERM{x and see if that helps.\n\r"
            "  {**{x Your client may not support them at all. If so, type {*THEME CONFIG DISABLE\n\r"
            "    RGB{x to hide them from the list of themes.\n\r"
            "  {**{x Likewise for 256 color indexed themes: type {*THEME CONFIG DISABLE 256{x\n\r"
            "    to hide them.\n\r"
            "  {**{x To hide this message in the future, type {*THEME CONFIG DISABLE RGB_HELP{x.\n\r"
            , ch);
    }
}

static void do_theme_palette(Mobile* ch, char* argument)
{
    static const char* help =
        "USAGE: {*THEME PALETTE <index> <color>{x\n\r"
        "\n\r"
        "{*index{x - Palette slot in the current theme to replace. Use THEME SHOW or \n\r"
        "        THEME PREVIEW for a list of current palette assignments.\n\r\n\r"
        "{*color{x - For ANSI color themes, {*<color>{x is a numeric value representing the \n\r"
        "        standard ANSI color palette index, {_or{x an ANSI color name. For 256-color\n\r"
        "        indexed themes, this is a numeric value from 0 to 256.\n\r\n\r"
        "        RGB themes (24-bit) use colors in the form {*#RRGGBB;{x.\n\r";

    if (!argument || !argument[0]) {
        send_to_char(help, ch);
        return;
    }

    ColorTheme* theme = ch->pcdata->current_theme;

    char arg1[MAX_INPUT_LENGTH];
    READ_ARG(arg1);

    if (!is_number(arg1) && theme->mode == COLOR_MODE_256) {
        send_to_char("{jPalette index must be a number.{x\n\r", ch);
        return;
    }

    int pal_idx = atoi(arg1);

    if (pal_idx < 0 || pal_idx >= PALETTE_SIZE) {
        printf_to_char(ch, "{jPalette index must be from 0 to %d.{x\n\r",
            PALETTE_SIZE);
        return;
    }

    bool color_is_num = is_number(argument);
    int idx = -1;

    if (color_is_num)
        idx = atoi(argument);

    Color color = { 0 };
    if (theme->mode == COLOR_MODE_16) {
        if (!color_is_num) {
            if (!lookup_color(argument, &color, ch))
                return;
        }
        else if (idx < 0 || idx > 16) {
            printf_to_char(ch, "{jYou much choose a color index from 0 to 15.{x\n\r");
            send_ansi_color_list(ch);
            return;
        }
        else
            set_color_ansi(&color, (uint8_t)idx / 8, (uint8_t)idx % 8);
    }
    else if (theme->mode == COLOR_MODE_256) {
        if (!color_is_num) {
            if (!lookup_color(argument, &color, ch))
                return;
        }
        else if (idx < 0 || idx > 255) {
            printf_to_char(ch, "{jYou much choose a color index from 0 to 255.{x\n\r");
            send_256_color_list(ch);
            return;
        }
        else
            set_color_256(&color, (uint8_t)idx);
    }
    else if (theme->mode == COLOR_MODE_RGB) {
        if (!lookup_color(argument, &color, ch))
            return;
    }
    else {
        printf_to_char(ch, "{jTheme palettes cannot refer to themselves. Choose a color.{x\n\r.");
    }

    free_string(theme->palette[pal_idx].cache);
    free_string(theme->palette[pal_idx].xterm);
    theme->palette[pal_idx] = color;
    theme->is_changed = true;
}

static void do_theme_preview(Mobile* ch, char* argument)
{
    char out[MAX_STRING_LENGTH];
    char buf[500];
    bool xterm = ch->pcdata->theme_config.xterm;

    static const char* help =
        "USAGE: {*THEME PREVIEW (@<player name>) <theme name>{x\n\r"
        "\n\r"
        "For a list of themes to preview, type '{*THEME LIST{x'.\n\r";

    if (!argument || !argument[0]) {
        send_to_char(help, ch);
        return;
    }

    ColorTheme* theme = lookup_color_theme(ch, argument);

    if (!theme) {
        printf_to_char(ch, "{jNo color theme named '%s' could be found.{x\n\r", 
            argument);
        return;
    }

    char* bg = BGCOLOR2STR(theme, xterm);

    sprintf(out, "%s\n\r", bg);

    sprintf(buf, "%sWelcome to Mud98 0.90. Please do not feed the mobiles.\n\r\n\r", COLOR2STR(theme, SLOT_TEXT, xterm));
    strcat(out, buf);
    
    sprintf(buf, "%sLimbo\n\r", COLOR2STR(theme, SLOT_ROOM_TITLE, xterm));
    strcat(out, buf);

    sprintf(buf,
        "%sYou are floating in a formless void, detached from all sensation of physical\n\r"
        "matter, surrounded by swirling glowing light, which fades into the relative\n\r"
        "darkness around you without any trace of edges or shadow.\n\r"
        "There is a \"No Tipping\" notice pinned to the darkness.\n\r",
        COLOR2STR(theme, SLOT_ROOM_TEXT, xterm));
    strcat(out, buf);

    sprintf(buf, "%sA dire wallabee is here.\n\r", COLOR2STR(theme, SLOT_ROOM_THINGS, xterm));
    strcat(out, buf);

    sprintf(buf, "%sYou have no unread notes.\n\r\n\r", COLOR2STR(theme, SLOT_INFO, xterm));
    strcat(out, buf);

    sprintf(buf, "%s<20/20hp 100/100m 100/100v 0gp/0sp [U]>\n\r\n\r", COLOR2STR(theme, SLOT_PROMPT, xterm));
    strcat(out, buf);

    sprintf(buf, "%sA dire wallabee says, '%sG'day, mate!%s'\n\r\n\r",
        COLOR2STR(theme, SLOT_SAY, xterm), COLOR2STR(theme, SLOT_SAY_TEXT, xterm),
        COLOR2STR(theme, SLOT_SAY, xterm));
    strcat(out, buf);

    sprintf(buf, "%sA dire wallabee tells you, '%sBut not really... TIME TO DIE!%s'\n\r\n\r",
        COLOR2STR(theme, SLOT_TELL, xterm), COLOR2STR(theme, SLOT_TELL_TEXT, xterm),
        COLOR2STR(theme, SLOT_TELL, xterm));
    strcat(out, buf);

    sprintf(buf, "%s<20/20hp 100/100m 100/100v 0gp/0sp [U]>\n\r\n\r", COLOR2STR(theme, SLOT_PROMPT, xterm));
    strcat(out, buf);

    sprintf(buf, "%sA dire wallabee's slash scratches you.\n\r", COLOR2STR(theme, SLOT_FIGHT_OHIT, xterm));
    strcat(out, buf);

    sprintf(buf, "%sYour slash grazes a dire wallabee.\n\r", COLOR2STR(theme, SLOT_FIGHT_YHIT, xterm));
    strcat(out, buf);

    sprintf(buf, "%sA dire wallabee is DEAD!!\n\r", COLOR2STR(theme, SLOT_FIGHT_DEATH, xterm));
    strcat(out, buf);

    sprintf(buf, "%sYou receive 199 experience points.\n\r", COLOR2STR(theme, SLOT_TEXT, xterm));
    strcat(out, buf);

    sprintf(buf, "%s\n\r{x\n\r", bg);
    strcat(out, buf);

    send_to_char(out, ch);
}

static void do_theme_remove(Mobile* ch, char* argument)
{
    if (!argument || !argument[0]) {
        send_to_char("{jYou specify the name of a color theme to remove.{x\n\r",
            ch);
        return;
    }

    ColorTheme* theme = lookup_local_color_theme(ch, argument);

    if (!theme) {
        send_to_char("{jYou do not have a color theme by that name.{x\n\r", ch);
        return;
    }

    if (str_cmp(argument, theme->name)) {
        // Lookup function uses str_prefix. Deleting stuff should be deliberate,
        // so force them to use the whole name.
        send_to_char("{jTo delete a color theme, you must use its full "
            "name.{x\n\r", ch);
        return;
    }

    if (!str_cmp(theme->name, ch->pcdata->current_theme->name)) {
        send_to_char("{jYou are currently using that color theme. Use {*THEME "
            "SELECT{j to switch themes before removing this one.{x\n\r", ch);
        return;
    }

    for (int i = 0; i < MAX_THEMES; ++i) {
        if (ch->pcdata->color_themes[i] == theme) {
            ch->pcdata->color_themes[i] = NULL;
            break;
        }
    }

    free_color_theme(theme);
    
    send_to_char("{jColor theme removed.{x\n\r", ch);
    return;
}

static void do_theme_save(Mobile* ch, char* argument)
{
    char* theme_name = ch->pcdata->theme_config.current_theme_name;
    ColorTheme* theme = ch->pcdata->current_theme;

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
            return;
        }
    }

    // It's new. Look for a new slot.
    for (int i = 0; i < MAX_THEMES; ++i) {
        if (!ch->pcdata->color_themes[i]) {
            ch->pcdata->color_themes[i] = dup_color_theme(theme);
            free_string(theme_name);
            ch->pcdata->theme_config.current_theme_name = str_dup(theme->name);
            save_char_obj(ch);
            return;
        }
    }
}

static void do_theme_select(Mobile* ch, char* argument)
{
    static const char* help =
        "USAGE: {*THEME SELECT (@<player>) <name>{x\n\r"
        "\n\r"
        "For a list of themes to select from, type 'THEME LIST'.\n\r";

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
            printf_to_char(ch, "{jNo color theme named '%s' could be found.{x\n\r",
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
            printf_to_char(ch, "{jSelecting another player's theme saves a copy "
                "to your own themes. You already have too many themes to add "
                "another.{x\n\r");
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

    printf_to_char(ch, "{jColor theme is now set to '{*%s{j'.{x\n\r", ch->pcdata->current_theme->name);
    printf_to_char(ch, "{_%s{x\n\r", ch->pcdata->current_theme->banner);
}

static void do_theme_set(Mobile* ch, char* argument)
{
    static const char* help =
        "USAGE: {*THEME SET NAME <name>\n\r"
        "       THEME SET BANNER <banner>\n\r"
        "       THEME SET PUBLIC\n\r"
        "       THEME SET PRIVATE{x\n\r"
        "\n\r";

    ColorTheme* theme = ch->pcdata->current_theme;

    if (theme == NULL) {
        send_to_char("{jYou do not have a color theme active. You can use "
            "{*THEME SELECT{j to choose one, or {*THEME CREATE{j to make a new "
            "one.{x\n\r", ch);
        return;
    }

    char opt[50] = { 0 };

    READ_ARG(opt);

    if (!opt[0] || !str_prefix(opt, "help")) {
        send_to_char(help, ch);
        return;
    }

    if (!str_prefix(opt, "name")) {
        if (!argument || !argument[0]) {
            send_to_char(help, ch);
            return;
        }

        size_t len = strlen(argument);
        if (len < 2 || len > 12) {
            send_to_char("{jTheme names must be between 2 and 12 characters "
                "long.{x\n\r", ch);
            return;
        }

        free_string(theme->name);
        theme->name = str_dup(argument);
        theme->is_changed = true;
    }
    else if (!str_prefix(opt, "banner")) {
        if (!argument || !argument[0]) {
            send_to_char(help, ch);
            return;
        }

        free_string(theme->banner);
        theme->banner = str_dup(argument);
        theme->is_changed = true;
    }
    else if (!str_prefix(opt, "public")) {
        if (!theme->is_public)
            theme->is_changed = true;
        theme->is_public = true;
        send_to_char("{jThis theme is now public.{x\n\r", ch);
    }
    else if (!str_prefix(opt, "private")) {
        if (theme->is_public)
            theme->is_changed = true;
        theme->is_public = false;
        send_to_char("{jThis theme is now private.{x\n\r", ch);
    }
    else
        send_to_char(help, ch);
}

static void do_theme_show(Mobile* ch, char* argument)
{
    ColorTheme* theme = ch->pcdata->current_theme;
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
    sprintf(out, "\n\r%s{_                                   00  02  04  06  08  10  12  14      {x\n\r", bg_code);

    // Second line, name and first row of color boxes
    sprintf(buf, "%s{t    %-30s", bg_code, theme->name);
    strcat(out, buf);
    for (int i = 0; i < PALETTE_SIZE; i += 2) {
        if (i <= theme->palette_max)
            sprintf(buf, "%s    ", bg_color_to_str(theme, &theme->palette[i], xterm));
        else
            sprintf(buf, "%s    ", bg_code);
        strcat(out, buf);
    }
    sprintf(buf, "%s    {x\n\r", bg_code);
    strcat(out, buf);

    // Third line, mode type and second row of color boxes
    sprintf(buf, "%s{*    %-20s          ", bg_code, mode);
    strcat(out, buf);
    for (int i = 1; i < PALETTE_SIZE; i += 2) {
        if (i <= theme->palette_max)
            sprintf(buf, "%s    ", bg_color_to_str(theme, &theme->palette[i], xterm));
        else
            sprintf(buf, "%s    ", bg_code);
        strcat(out, buf);
    }
    sprintf(buf, "%s    {x\n\r", bg_code);
    strcat(out, buf);

    // Fourth line, blank BG color
    sprintf(buf, "%s{_                                   01  03  05  07  09  11  13  15      {x\n\r\n\r", bg_code);
    strcat(out, buf);

    int row = 0;
    strcat(out, "{TPalette Swatches:{x (change with {*THEME PALETTE{x)\n\r");
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
            sprintf(buf, "    %s%02d %-15s {x", color_to_str(theme, color, xterm),
                i, color_ref);
        else {
            sprintf(buf, "    %s%s%02d %-15s {x",
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
    strcat(out, "{TChannel Assignments:{x (change with {*THEME CHANNEL{x)\n\r");
    for (int i = 0; i < SLOT_MAX; ++i) {
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
            sprintf(buf, "    {%c{{%c %-15s %-15s {x", channel->code, channel->code,
                channel->name, color_ref);
        else {
            sprintf(buf, "    %s%s{{%c %-15s %-15s {x", 
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

void do_theme(Mobile* ch, char* argument)
{
    static const char* help =
        "USAGE: {*THEME CONFIG\n\r"
        "       THEME CHANNEL/CHAN <channel> <color>\n\r"
        "       THEME CREATE (ANSI|256|RGB) <name>\n\r"
        "       THEME DISCARD\n\r"
        "       THEME LIST (ALL|SYSTEM|PUBLIC|PRIVATE)\n\r"
        "       THEME PALETTE/PAL <index> <color>\n\r"
        "       THEME PREVIEW <name>\n\r"
        "       THEME REMOVE <name>\n\r"
        "       THEME SAVE\n\r"
        "       THEME SELECT <name>\n\r"
        "       THEME SET <more options>\n\r"
        "       THEME SHOW{x\n\r"
        "\n\r"
        "Type '{*THEME <option>{x' for more information.\n\r";

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
        return;
    }

    if (!str_prefix(cmd, "channel")) {
        do_theme_channel(ch, argument);
    }
    else if (!str_prefix(cmd, "config")) {
        do_theme_config(ch, argument);
    }
    else if (!str_prefix(cmd, "create")) {
        do_theme_create(ch, argument);
    }
    else if (!str_cmp(cmd, "discard")) {
        do_theme_discard(ch, argument);
    }
    else if (!str_prefix(cmd, "list")) {
        do_theme_list(ch, argument);
    }
    else if (!str_prefix(cmd, "palette")) {
        do_theme_palette(ch, argument);
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

    for (int i = 0; i < SLOT_MAX; ++i) {
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

    for (int i = 0; i < SYSTEM_COLOR_THEME_MAX; ++i) {
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

            if (str_prefix(name, wch->name))
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

        printf_to_char(ch, "{jCould not find the theme '%s' or person '%s'.\n\r",
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

    ch->pcdata->current_theme = dup_color_theme(system_color_themes[SYSTEM_COLOR_THEME_LOPE]);
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

const ColorChannelEntry color_slot_entries[SLOT_MAX] = {
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
    { "wiznet",			    SLOT_WIZNET,	        'B' },
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
};

////////////////////////////////////////////////////////////////////////////////
// Theme Definitions
////////////////////////////////////////////////////////////////////////////////

// I can't use the colors as they are in color.h because the cast to Color 
// messes with the C-struct literal initialization.

#define SLOT_0                      THEME_PAL_REF(0)
#define SLOT_1                      THEME_PAL_REF(1)
#define SLOT_2                      THEME_PAL_REF(2)
#define SLOT_3                      THEME_PAL_REF(3)
#define SLOT_4                      THEME_PAL_REF(4)
#define SLOT_5                      THEME_PAL_REF(5)
#define SLOT_6                      THEME_PAL_REF(6)
#define SLOT_7                      THEME_PAL_REF(7)
#define SLOT_8                      THEME_PAL_REF(8)
#define SLOT_9                      THEME_PAL_REF(9)
#define SLOT_A                      THEME_PAL_REF(10)
#define SLOT_B                      THEME_PAL_REF(11)
#define SLOT_C                      THEME_PAL_REF(12)
#define SLOT_D                      THEME_PAL_REF(13)
#define SLOT_E                      THEME_PAL_REF(14)
#define SLOT_F                      THEME_PAL_REF(15)

#define VT_256_FG                   "\033[38;5;"
#define VT_24B_FG                   "\033[38:2::"
#define XTERM_24B_FG                "\033[38;2;"

#define THEME_ENTRY_ANSI(l, c, s)       {.mode = COLOR_MODE_16, \
    .code = { l, c, 0 }, .cache = s, .xterm = NULL }

#define THEME_ENTRY_256(i, s)           {.mode = COLOR_MODE_256, \
    .code = { i, 0, 0 }, .cache = s, .xterm = NULL }

#define THEME_ENTRY_RGB(r, g, b, s, x)  {.mode = COLOR_MODE_RGB, \
    .code = { r, g, b }, .cache = s, .xterm = x }

#define THEME_PAL_REF(s)                {.mode = COLOR_MODE_PAL_IDX, \
    .code = { s, 0, 0 }, .cache = NULL, .xterm = NULL }

////////////////////////////////////////////////////////////////////////////////
// Lope's OG ColoUr Code Theme; default for Mud98
////////////////////////////////////////////////////////////////////////////////

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

const AnsiPaletteEntry ansi_palette[15] = {
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

const ColorTheme theme_lope = {
    .name = "Lope",
    .type = COLOR_THEME_TYPE_SYSTEM,
    .mode = COLOR_MODE_16,
    .banner =
        "Lope's ColoUr Code 2.0\n\r"
        "ColoUr is brought to you by Lope, ant@solace.mh.se\n\r",
    .palette = {
        ANSI_WHITE,         // 0: default text color
        ANSI_BLACK,         // 1: default background
        ANSI_D_GRAY,        // 2: default alt-text #1
        ANSI_B_WHITE,       // 3: default alt-text #3
        ANSI_RED,           // 4
        ANSI_B_RED,         // 5
        ANSI_GREEN,         // 6
        ANSI_B_GREEN,       // 7
        ANSI_YELLOW,        // 8
        ANSI_B_YELLOW,      // 9
        ANSI_BLUE,          // A
        ANSI_B_BLUE,        // B
        ANSI_MAGENTA,       // C
        ANSI_B_MAGENTA,     // D
        ANSI_CYAN,          // E
        ANSI_B_CYAN,        // F
    },
    .palette_max = ANSI_MAX,
    .channels = {
        SLOT_0,             // text
        SLOT_1,             // background
        SLOT_3,             // Title     
        SLOT_E,             // Alt-Text 1
        SLOT_2,             // Alt-Text 2
        SLOT_B,             // Decor 1   
        SLOT_9,             // Decor 2   
        SLOT_E,             // Decor 3   
        SLOT_9,             // auction
        SLOT_3,             // auction_text
        SLOT_C,             // gossip
        SLOT_D,             // gossip_text
        SLOT_4,             // music
        SLOT_5,             // music_text
        SLOT_9,             // question
        SLOT_3,             // question_text
        SLOT_9,             // answer
        SLOT_3,             // answer_text
        SLOT_6,             // quote
        SLOT_7,             // quote_text
        SLOT_E,             // immtalk_text
        SLOT_8,             // immtalk_type
        SLOT_8,             // info
        SLOT_6,             // say
        SLOT_7,             // say_text
        SLOT_6,             // tell
        SLOT_7,             // tell_text
        SLOT_6,             // reply
        SLOT_7,             // reply_text
        SLOT_6,             // gtell_text
        SLOT_4,             // gtell_type
        SLOT_6,             // wiznet
        SLOT_E,             // room_title
        SLOT_0,             // room_text
        SLOT_6,             // room_exits
        SLOT_E,             // room_things
        SLOT_E,             // prompt
        SLOT_4,             // fight_death
        SLOT_6,             // fight_yhit
        SLOT_8,             // fight_ohit
        SLOT_4,             // fight_thit
        SLOT_0,             // fight_skill
    }
};

////////////////////////////////////////////////////////////////////////////////
// Monorail
////////////////////////////////////////////////////////////////////////////////

#define MONO_0          THEME_ENTRY_256(15,    VT_256_FG "15m"     )
#define MONO_1          THEME_ENTRY_256(0,     VT_256_FG "0m"      )
#define MONO_2          THEME_ENTRY_256(244,   VT_256_FG "244m"    )
#define MONO_3          THEME_ENTRY_256(242,   VT_256_FG "242m"    )
#define MONO_4          THEME_ENTRY_256(246,   VT_256_FG "246m"    )
#define MONO_5          THEME_ENTRY_256(240,   VT_256_FG "240m"    )
#define MONO_6          THEME_ENTRY_256(248,   VT_256_FG "248m"    )
#define MONO_7          THEME_ENTRY_256(238,   VT_256_FG "238m"    )
#define MONO_8          THEME_ENTRY_256(250,   VT_256_FG "250m"    )
#define MONO_9          THEME_ENTRY_256(236,   VT_256_FG "236m"    )
#define MONO_A          THEME_ENTRY_256(252,   VT_256_FG "252m"    )
#define MONO_B          THEME_ENTRY_256(234,   VT_256_FG "234m"    )
#define MONO_C          THEME_ENTRY_256(254,   VT_256_FG "254m"    )
#define MONO_D          THEME_ENTRY_256(233,   VT_256_FG "233m"    )
#define MONO_E          THEME_ENTRY_256(255,   VT_256_FG "255m"    )
#define MONO_F          THEME_ENTRY_256(232,   VT_256_FG "232m"    )
#define MONO_MAX        16

const ColorTheme theme_mono = {
    .name = "Monorail",
    .type = COLOR_THEME_TYPE_SYSTEM,
    .mode = COLOR_MODE_256,
    .banner = "",
    .palette = {
        MONO_0,             // 0: default text color
        MONO_1,             // 1: default background
        MONO_2,             // 2: default alt-text #1
        MONO_3,             // 3: default alt-text #3
        MONO_4,             // 4
        MONO_5,             // 5
        MONO_6,             // 6
        MONO_7,             // 7
        MONO_8,             // 8
        MONO_9,             // 9
        MONO_A,             // A
        MONO_B,             // B
        MONO_C,             // C
        MONO_D,             // D
        MONO_E,             // E
        MONO_F,             // F
    },
    .palette_max = MONO_MAX,
    .channels = {
        SLOT_0,             // text
        SLOT_1,             // background
        SLOT_3,             // Title     
        SLOT_E,             // Alt-Text 1
        SLOT_2,             // Alt-Text 2
        SLOT_B,             // Decor 1   
        SLOT_9,             // Decor 2   
        SLOT_E,             // Decor 3   
        SLOT_9,             // auction
        SLOT_3,             // auction_text
        SLOT_C,             // gossip
        SLOT_D,             // gossip_text
        SLOT_4,             // music
        SLOT_5,             // music_text
        SLOT_9,             // question
        SLOT_3,             // question_text
        SLOT_9,             // answer
        SLOT_3,             // answer_text
        SLOT_6,             // quote
        SLOT_7,             // quote_text
        SLOT_E,             // immtalk_text
        SLOT_8,             // immtalk_type
        SLOT_8,             // info
        SLOT_6,             // say
        SLOT_7,             // say_text
        SLOT_6,             // tell
        SLOT_7,             // tell_text
        SLOT_6,             // reply
        SLOT_7,             // reply_text
        SLOT_6,             // gtell_text
        SLOT_4,             // gtell_type
        SLOT_6,             // wiznet
        SLOT_E,             // room_title
        SLOT_0,             // room_text
        SLOT_6,             // room_exits
        SLOT_E,             // room_things
        SLOT_E,             // prompt
        SLOT_4,             // fight_death
        SLOT_6,             // fight_yhit
        SLOT_8,             // fight_ohit
        SLOT_4,             // fight_thit
        SLOT_0,             // fight_skill
    }
};

////////////////////////////////////////////////////////////////////////////////
// Cool Cat Theme
////////////////////////////////////////////////////////////////////////////////

#define COOL_CAT_TEXT           THEME_ENTRY_RGB(0xF7u, 0xF7u, 0xF3u, VT_24B_FG "247:247:243m",  XTERM_24B_FG "247;247;243m"    )
#define COOL_CAT_BG             THEME_ENTRY_RGB(0x28u, 0x29u, 0x21u, VT_24B_FG "40:41:33m",     XTERM_24B_FG "40;41;33m"       )
#define COOL_CAT_ALT_TEXT       THEME_ENTRY_RGB(0x75u, 0x71u, 0x5Eu, VT_24B_FG "117:113:94m",   XTERM_24B_FG "117;113;94m"     )
#define COOL_CAT_PINK           THEME_ENTRY_RGB(0xFAu, 0x25u, 0x73u, VT_24B_FG "250:37:115m",   XTERM_24B_FG "250;37;115m"     )
#define COOL_CAT_L_PINK         THEME_ENTRY_RGB(0xFFu, 0x75u, 0xA3u, VT_24B_FG "255:117:163m",  XTERM_24B_FG "255;117;163m"    )
#define COOL_CAT_ORANGE         THEME_ENTRY_RGB(0xFDu, 0x97u, 0x71u, VT_24B_FG "253:151:113m",  XTERM_24B_FG "253;151;113m"    )
#define COOL_CAT_YELLOW         THEME_ENTRY_RGB(0xFFu, 0xD9u, 0x67u, VT_24B_FG "255:217:103m",  XTERM_24B_FG "255;217;103m"    )
#define COOL_CAT_GREEN          THEME_ENTRY_RGB(0xA5u, 0xE3u, 0x2Fu, VT_24B_FG "165:227:47m",   XTERM_24B_FG "165;227;47m"     )
#define COOL_CAT_L_GREEN        THEME_ENTRY_RGB(0xC5u, 0xFFu, 0x6Fu, VT_24B_FG "197:255:111m",  XTERM_24B_FG "197;255;111m"    )
#define COOL_CAT_BLUE           THEME_ENTRY_RGB(0x65u, 0xD9u, 0xF0u, VT_24B_FG "101:217:240m",  XTERM_24B_FG "101;217;240m"    )
#define COOL_CAT_L_BLUE         THEME_ENTRY_RGB(0x9Au, 0xF8u, 0xFFu, VT_24B_FG "154:248:255m",  XTERM_24B_FG "154;248;255m"    )
#define COOL_CAT_PURPLE         THEME_ENTRY_RGB(0xA0u, 0x32u, 0xFFu, VT_24B_FG "154:50:255m",   XTERM_24B_FG "154;50;255m"     )
#define COOL_CAT_L_PURPLE       THEME_ENTRY_RGB(0xAFu, 0x80u, 0xFFu, VT_24B_FG "175:128:255m",  XTERM_24B_FG "175;128;255m"    )
#define COOL_CAT_MAX            15

const ColorTheme theme_cool_cat = {
    .name = "Cool Cat",
    .type = COLOR_THEME_TYPE_SYSTEM,
    .mode = COLOR_MODE_RGB,
    .banner =
        "If your MUD client/terminal does not support server-initiated changes "
        "to background color, consider manually setting your backround to " 
        "to #282921 for best results.\n\r",
    .palette = {
        COOL_CAT_TEXT,      // 0: default text color
        COOL_CAT_BG,        // 1: default background
        COOL_CAT_ALT_TEXT,  // 2: default alt-text #1
        COOL_CAT_ALT_TEXT,  // 3: default alt-text #3
        COOL_CAT_PINK,      // 4
        COOL_CAT_L_PINK,    // 5
        COOL_CAT_GREEN,     // 6
        COOL_CAT_L_GREEN,   // 7
        COOL_CAT_ORANGE,    // 8
        COOL_CAT_YELLOW,    // 9
        COOL_CAT_BLUE,      // A
        COOL_CAT_L_BLUE,    // B
        COOL_CAT_PURPLE,    // C
        COOL_CAT_L_PURPLE,  // D
        COOL_CAT_BLUE,      // E
        COOL_CAT_L_BLUE,    // F
    },
    .palette_max = COOL_CAT_MAX,
    .channels = {
        SLOT_0,             // text
        SLOT_1,             // background
        SLOT_3,             // Title     
        SLOT_E,             // Alt-Text 1
        SLOT_2,             // Alt-Text 2
        SLOT_B,             // Decor 1   
        SLOT_9,             // Decor 2   
        SLOT_E,             // Decor 3   
        SLOT_9,             // auction
        SLOT_3,             // auction_text
        SLOT_C,             // gossip
        SLOT_D,             // gossip_text
        SLOT_4,             // music
        SLOT_5,             // music_text
        SLOT_9,             // question
        SLOT_3,             // question_text
        SLOT_9,             // answer
        SLOT_3,             // answer_text
        SLOT_6,             // quote
        SLOT_7,             // quote_text
        SLOT_E,             // immtalk_text
        SLOT_8,             // immtalk_type
        SLOT_8,             // info
        SLOT_6,             // say
        SLOT_7,             // say_text
        SLOT_6,             // tell
        SLOT_7,             // tell_text
        SLOT_6,             // reply
        SLOT_7,             // reply_text
        SLOT_6,             // gtell_text
        SLOT_4,             // gtell_type
        SLOT_6,             // wiznet
        SLOT_E,             // room_title
        SLOT_0,             // room_text
        SLOT_6,             // room_exits
        SLOT_E,             // room_things
        SLOT_E,             // prompt
        SLOT_4,             // fight_death
        SLOT_6,             // fight_yhit
        SLOT_8,             // fight_ohit
        SLOT_4,             // fight_thit
        SLOT_0,             // fight_skill
    }
};

////////////////////////////////////////////////////////////////////////////////
// Synthwave City
////////////////////////////////////////////////////////////////////////////////

#define CITY_WHITE          THEME_ENTRY_RGB(0xF6u, 0xEDu, 0xDBu, VT_24B_FG "246:237:219m",  XTERM_24B_FG "246;237;219m"    )
#define CITY_BLACK          THEME_ENTRY_RGB(0x1Bu, 0x1Eu, 0x23u, VT_24B_FG "27:30:35m",     XTERM_24B_FG "27;30;35m"       )
#define CITY_ORANGE         THEME_ENTRY_RGB(0xECu, 0x8Du, 0x75u, VT_24B_FG "236:141:117m",  XTERM_24B_FG "236;141;117m"    )
#define CITY_RED            THEME_ENTRY_RGB(0xBDu, 0x4Bu, 0x64u, VT_24B_FG "189:75:100m",   XTERM_24B_FG "189;75;100m"     )
#define CITY_PINK           THEME_ENTRY_RGB(0x9Eu, 0x22u, 0x81u, VT_24B_FG "158:34:129m",   XTERM_24B_FG "158;34;129m"     )
#define CITY_PURPLE         THEME_ENTRY_RGB(0x40u, 0x26u, 0x5Cu, VT_24B_FG "64:38:92m",     XTERM_24B_FG "64;38;92m"       )
#define CITY_BLUE           THEME_ENTRY_RGB(0x24u, 0x45u, 0x84u, VT_24B_FG "36:69:132m",    XTERM_24B_FG "36;69;132m"      )
#define CITY_SKY            THEME_ENTRY_RGB(0x50u, 0xA9u, 0xCFu, VT_24B_FG "80:169:207m",   XTERM_24B_FG "80;169;207m"     )
#define CITY_TEAL           THEME_ENTRY_RGB(0x96u, 0xE6u, 0xC2u, VT_24B_FG "150:230:194m",  XTERM_24B_FG "150;230;194m"    )
#define CITY_MAX            15

const ColorTheme theme_synthwave_city = {
    .name = "Synthwave City",
    .type = COLOR_THEME_TYPE_SYSTEM,
    .mode = COLOR_MODE_RGB,
    .banner =
        "If your MUD client/terminal does not support server-initiated changes "
        "to background color, consider manually setting your backround to "
        "to #1B1E23 for best results.\n\r",
    .palette = {
        CITY_WHITE,         // 0
        CITY_BLACK,         // 1
        CITY_ORANGE,        // 2
        CITY_SKY,           // 3
        CITY_RED,           // 4
        CITY_ORANGE,        // 5
        CITY_SKY,           // 6
        CITY_TEAL,          // 7
        CITY_ORANGE,        // 8
        CITY_WHITE,         // 9
        CITY_PURPLE,        // A
        CITY_BLUE,          // B
        CITY_PURPLE,        // C
        CITY_PINK,          // D
        CITY_BLUE,          // E
        CITY_TEAL,          // F
    },
    .palette_max = CITY_MAX,
    .channels = {
        SLOT_0,             // text
        SLOT_1,             // background
        SLOT_3,             // Title     
        SLOT_E,             // Alt-Text 1
        SLOT_2,             // Alt-Text 2
        SLOT_2,             // Decor 1   
        SLOT_3,             // Decor 2   
        SLOT_4,             // Decor 3   
        SLOT_9,             // auction
        SLOT_3,             // auction_text
        SLOT_C,             // gossip
        SLOT_D,             // gossip_text
        SLOT_4,             // music
        SLOT_5,             // music_text
        SLOT_9,             // question
        SLOT_3,             // question_text
        SLOT_9,             // answer
        SLOT_3,             // answer_text
        SLOT_6,             // quote
        SLOT_7,             // quote_text
        SLOT_E,             // immtalk_text
        SLOT_8,             // immtalk_type
        SLOT_8,             // info
        SLOT_6,             // say
        SLOT_7,             // say_text
        SLOT_6,             // tell
        SLOT_7,             // tell_text
        SLOT_6,             // reply
        SLOT_7,             // reply_text
        SLOT_6,             // gtell_text
        SLOT_4,             // gtell_type
        SLOT_6,             // wiznet
        SLOT_E,             // room_title
        SLOT_0,             // room_text
        SLOT_6,             // room_exits
        SLOT_E,             // room_things
        SLOT_E,             // prompt
        SLOT_4,             // fight_death
        SLOT_6,             // fight_yhit
        SLOT_8,             // fight_ohit
        SLOT_4,             // fight_thit
        SLOT_0,             // fight_skill
    }
};

////////////////////////////////////////////////////////////////////////////////
// Glow Pop
////////////////////////////////////////////////////////////////////////////////

#define POP_WHITE           THEME_ENTRY_RGB(0xFAu, 0xFDu, 0xFFu, VT_24B_FG "250:253:255m",  XTERM_24B_FG "250;253;255m"    )   // #FAFDFF White
#define POP_BLACK           THEME_ENTRY_RGB(0x16u, 0x17u, 0x1Au, VT_24B_FG "22:23:26m",     XTERM_24B_FG "22;23;26m"       )   // #16171A Black
#define POP_SBLUE           THEME_ENTRY_RGB(0x23u, 0x49u, 0x75u, VT_24B_FG "35:73:117m",    XTERM_24B_FG "35;73;117m"      )   // #234975 Slate Blue
#define POP_LSBLUE          THEME_ENTRY_RGB(0x00u, 0x78u, 0x99u, VT_24B_FG "0:120:153m",    XTERM_24B_FG "0;120;153m"      )   // #007899 L. Slate Blue
#define POP_CRIMSON         THEME_ENTRY_RGB(0x7Fu, 0x06u, 0x22u, VT_24B_FG "127:6:34m",     XTERM_24B_FG "127;6;34m"       )   // #7F0622 D. Red
#define POP_RED             THEME_ENTRY_RGB(0xD6u, 0x24u, 0x11u, VT_24B_FG "214:36:17m",    XTERM_24B_FG "214;36;17m"      )   // #D62411 Red
#define POP_S_GREEN         THEME_ENTRY_RGB(0x10u, 0xD2u, 0x75u, VT_24B_FG "16:210:117m",   XTERM_24B_FG "16;210;117m"     )   // #10D275 Sea Green
#define POP_L_GREEN         THEME_ENTRY_RGB(0xBFu, 0xFFu, 0x3Cu, VT_24B_FG "191:255:60m",   XTERM_24B_FG "191;255;60m"     )   // #BFFF3C Lime Green
#define POP_ORANGE          THEME_ENTRY_RGB(0xFFu, 0x84u, 0x26u, VT_24B_FG "255:132:38m",   XTERM_24B_FG "255;132;38m"     )   // #FF8426 Orange
#define POP_SUNFLOWER       THEME_ENTRY_RGB(0xFFu, 0xD1u, 0x00u, VT_24B_FG "255:209:0m",    XTERM_24B_FG "255;209;0m"      )   // #FFD100 Sunflower
#define POP_BLUE            THEME_ENTRY_RGB(0x00u, 0x28u, 0x59u, VT_24B_FG "0:40:89m",      XTERM_24B_FG "0;40;89m"        )   // #002859 D. Blue
#define POP_SKY_BLUE        THEME_ENTRY_RGB(0x68u, 0xAEu, 0xD4u, VT_24B_FG "104:174:212m",  XTERM_24B_FG "104;174;212m"    )   // #68AED4 Sky Blue
#define POP_GRAPE           THEME_ENTRY_RGB(0x43u, 0x00u, 0x67u, VT_24B_FG "67:0:103m",     XTERM_24B_FG "67;0;103m"       )   // #430067 Violet
#define POP_VIOLET          THEME_ENTRY_RGB(0x94u, 0x21u, 0x6Au, VT_24B_FG "148:33:106m",   XTERM_24B_FG "148;33;106m"     )   // #94216A Grape
#define POP_H_PINK          THEME_ENTRY_RGB(0xFFu, 0x26u, 0x74u, VT_24B_FG "255:38:116m",   XTERM_24B_FG "255;38;116m"     )   // #FF2674 Hot Pink
#define POP_L_PINK          THEME_ENTRY_RGB(0xFFu, 0x80u, 0xA4u, VT_24B_FG "255:128:164m",  XTERM_24B_FG "255;128;164m"    )   // #FF80A4 L. Pink
#define POP_MAX             16

const ColorTheme theme_glow_pop = {
    .name = "Glow Pop",
    .type = COLOR_THEME_TYPE_SYSTEM,
    .mode = COLOR_MODE_RGB,
    .banner =
        "If your MUD client/terminal does not support server-initiated changes "
        "to background color, consider manually setting your backround to "
        "to #16171A for best results.\n\r",
    .palette = {
        POP_WHITE,          // 0
        POP_BLACK,          // 1
        POP_SBLUE,          // 2
        POP_LSBLUE,         // 3
        POP_CRIMSON,        // 4
        POP_RED,            // 5
        POP_S_GREEN,        // 6
        POP_L_GREEN,        // 7
        POP_ORANGE,         // 8
        POP_SUNFLOWER,      // 9
        POP_BLUE,           // A
        POP_SKY_BLUE,       // B
        POP_GRAPE,          // C
        POP_VIOLET,         // D
        POP_H_PINK,         // E
        POP_L_PINK,         // F
    },
    .palette_max = POP_MAX,
    .channels = {
        SLOT_0,             // text
        SLOT_1,             // background
        SLOT_3,             // Title     
        SLOT_E,             // Alt-Text 1
        SLOT_2,             // Alt-Text 2
        SLOT_B,             // Decor 1   
        SLOT_9,             // Decor 2   
        SLOT_E,             // Decor 3   
        SLOT_9,             // auction
        SLOT_3,             // auction_text
        SLOT_C,             // gossip
        SLOT_D,             // gossip_text
        SLOT_4,             // music
        SLOT_5,             // music_text
        SLOT_9,             // question
        SLOT_3,             // question_text
        SLOT_9,             // answer
        SLOT_3,             // answer_text
        SLOT_6,             // quote
        SLOT_7,             // quote_text
        SLOT_E,             // immtalk_text
        SLOT_8,             // immtalk_type
        SLOT_8,             // info
        SLOT_6,             // say
        SLOT_7,             // say_text
        SLOT_6,             // tell
        SLOT_7,             // tell_text
        SLOT_6,             // reply
        SLOT_7,             // reply_text
        SLOT_6,             // gtell_text
        SLOT_4,             // gtell_type
        SLOT_6,             // wiznet
        SLOT_E,             // room_title
        SLOT_0,             // room_text
        SLOT_6,             // room_exits
        SLOT_E,             // room_things
        SLOT_E,             // prompt
        SLOT_4,             // fight_death
        SLOT_6,             // fight_yhit
        SLOT_8,             // fight_ohit
        SLOT_4,             // fight_thit
        SLOT_0,             // fight_skill
    }
};

////////////////////////////////////////////////////////////////////////////////

const ColorTheme* system_color_themes[SYSTEM_COLOR_THEME_MAX] = {
    &theme_lope,
    &theme_mono,
    &theme_cool_cat,
    &theme_synthwave_city,
    &theme_glow_pop,
};
