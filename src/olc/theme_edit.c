////////////////////////////////////////////////////////////////////////////////
// olc/theme_edit.c
////////////////////////////////////////////////////////////////////////////////

#include <merc.h>

#include "olc.h"

#include <color.h>
#include <comm.h>
#include <handler.h>
#include <interp.h>
#include <save.h>

#include <entities/player_data.h>

#define THEMEEDIT(fun) bool fun(Mobile *ch, char *argument)

static bool theme_edit_check(Mobile* ch, ColorTheme** out_theme)
{
    if (!ch->desc || ch->desc->editor != ED_THEME || ch->desc->pEdit == 0) {
        send_to_char(COLOR_INFO "You are not currently editing a theme." COLOR_EOL, ch);
        edit_done(ch);
        return false;
    }

    EDIT_THEME(ch, *out_theme);
    if (!*out_theme) {
        send_to_char(COLOR_INFO "No color theme is loaded for editing." COLOR_EOL, ch);
        edit_done(ch);
        return false;
    }

    return true;
}

THEMEEDIT(theme_edit_name);
THEMEEDIT(theme_edit_banner);
THEMEEDIT(theme_edit_public);
THEMEEDIT(theme_edit_private);
THEMEEDIT(theme_edit_channel);
THEMEEDIT(theme_edit_palette);
THEMEEDIT(theme_edit_resize);
THEMEEDIT(theme_edit_show_cmd);
THEMEEDIT(theme_edit_preview_cmd);
THEMEEDIT(theme_edit_save);
THEMEEDIT(theme_edit_discard);

const OlcCmd theme_edit_table[] = {
    { "name",        theme_edit_name },
    { "banner",      theme_edit_banner },
    { "public",      theme_edit_public },
    { "private",     theme_edit_private },
    { "channel",     theme_edit_channel },
    { "palette",     theme_edit_palette },
    { "resize",      theme_edit_resize },
    { "show",        theme_edit_show_cmd },
    { "preview",     theme_edit_preview_cmd },
    { "save",        theme_edit_save },
    { "discard",     theme_edit_discard },
    { "commands",    show_commands },
    { NULL,           0 }
};

void theme_edit(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;

    strcpy(arg, argument ? argument : "");
    READ_ARG(command);

    if (ch->pcdata == NULL) {
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        theme_edit_show_cmd(ch, argument);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    for (cmd = 0; theme_edit_table[cmd].name != NULL; ++cmd) {
        if (!str_prefix(command, theme_edit_table[cmd].name)) {
            if ((*theme_edit_table[cmd].olc_fun)(ch, argument) && ch->pcdata->current_theme)
                ch->pcdata->current_theme->is_changed = true;
            return;
        }
    }

    interpret(ch, arg);
}

THEMEEDIT(theme_edit_name)
{
    ColorTheme* theme;
    if (!theme_edit_check(ch, &theme))
        return false;

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "USAGE: name <theme name>" COLOR_EOL, ch);
        return false;
    }

    size_t len = strlen(argument);
    if (len < 2 || len > 12) {
        send_to_char(COLOR_INFO "Theme names must be between 2 and 12 characters long." COLOR_EOL, ch);
        return false;
    }

    free_string(theme->name);
    theme->name = str_dup(argument);
    theme->is_changed = true;
    send_to_char(COLOR_INFO "Theme renamed." COLOR_EOL, ch);
    return true;
}

THEMEEDIT(theme_edit_banner)
{
    ColorTheme* theme;
    if (!theme_edit_check(ch, &theme))
        return false;

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "USAGE: banner <text>" COLOR_EOL, ch);
        return false;
    }

    free_string(theme->banner);
    theme->banner = str_dup(argument);
    theme->is_changed = true;
    send_to_char(COLOR_INFO "Theme banner updated." COLOR_EOL, ch);
    return true;
}

THEMEEDIT(theme_edit_public)
{
    ColorTheme* theme;
    if (!theme_edit_check(ch, &theme))
        return false;

    if (!theme->is_public) {
        theme->is_public = true;
        theme->is_changed = true;
    }
    send_to_char(COLOR_INFO "This theme is now public." COLOR_EOL, ch);
    return true;
}

THEMEEDIT(theme_edit_private)
{
    ColorTheme* theme;
    if (!theme_edit_check(ch, &theme))
        return false;

    if (theme->is_public) {
        theme->is_public = false;
        theme->is_changed = true;
    }
    send_to_char(COLOR_INFO "This theme is now private." COLOR_EOL, ch);
    return true;
}

THEMEEDIT(theme_edit_channel)
{
    ColorTheme* theme;
    if (!theme_edit_check(ch, &theme))
        return false;

    static const char* help =
        COLOR_INFO
        "USAGE: " COLOR_ALT_TEXT_1 "channel <channel> <color>" COLOR_CLEAR "\n\r\n\r"
        COLOR_ALT_TEXT_1 "channel" COLOR_INFO " - Name of the channel to change. Use SHOW for a list." COLOR_EOL
        COLOR_ALT_TEXT_1 "color" COLOR_INFO "  - ANSI themes accept indexes (0-15) or names. 256/24-bit themes\n\r"
        "             use 0-255 or #RRGGBB; values." COLOR_EOL;

    char chan_arg[MAX_INPUT_LENGTH];
    READ_ARG(chan_arg);
    if (!chan_arg[0]) {
        send_to_char(help, ch);
        return false;
    }

    int chan = -1;
    LOOKUP_COLOR_SLOT_NAME(chan, chan_arg);
    if (chan < 0) {
        printf_to_char(ch, COLOR_INFO "'%s' is not a valid channel name." COLOR_EOL, chan_arg);
        return false;
    }

    bool color_is_num = is_number(argument);
    int idx = color_is_num ? atoi(argument) : -1;

    Color color = { 0 };
    if (theme->mode == COLOR_MODE_16) {
        if (!color_is_num) {
            if (!lookup_color(argument, &color, ch))
                return false;
        }
        else if (idx < 0 || idx > 16) {
            send_to_char(COLOR_INFO "Choose a color index from 0 to 15." COLOR_EOL, ch);
            return false;
        }
        else
            set_color_ansi(&color, (uint8_t)idx / 8, (uint8_t)idx % 8);
    }
    else if (theme->mode == COLOR_MODE_256) {
        if (!color_is_num) {
            if (!lookup_color(argument, &color, ch))
                return false;
        }
        else if (idx < 0 || idx > 255) {
            send_to_char(COLOR_INFO "Choose a color index from 0 to 255." COLOR_EOL, ch);
            send_256_color_list(ch);
            return false;
        }
        else
            set_color_256(&color, (uint8_t)idx);
    }
    else if (theme->mode == COLOR_MODE_RGB) {
        if (!lookup_color(argument, &color, ch))
            return false;
    }
    else {
        send_to_char(COLOR_INFO "Theme palettes cannot refer to themselves. Choose a color." COLOR_EOL, ch);
        return false;
    }

    free_string(theme->channels[chan].cache);
    theme->channels[chan].cache = NULL;
    free_string(theme->channels[chan].xterm);
    theme->channels[chan].xterm = NULL;
    theme->channels[chan] = color;
    theme->is_changed = true;
    send_to_char(COLOR_INFO "Channel updated." COLOR_EOL, ch);
    return true;
}

THEMEEDIT(theme_edit_palette)
{
    ColorTheme* theme;
    if (!theme_edit_check(ch, &theme))
        return false;

    static const char* help =
        COLOR_INFO
        "USAGE: " COLOR_ALT_TEXT_1 "palette <index> <color>" COLOR_CLEAR "\n\r\n\r"
        COLOR_ALT_TEXT_1 "index" COLOR_INFO " - Palette slot to change (0 to 15)." COLOR_EOL
        COLOR_ALT_TEXT_1 "color" COLOR_INFO " - Use the same formats accepted by the CHANNEL command." COLOR_EOL;

    char arg1[MAX_INPUT_LENGTH];
    READ_ARG(arg1);
    if (!is_number(arg1) || !argument || !argument[0]) {
        send_to_char(help, ch);
        return false;
    }

    int pal_idx = atoi(arg1);
    if (pal_idx < 0 || pal_idx >= theme->palette_max) {
        printf_to_char(ch, COLOR_INFO "Palette index must be between 0 and %d. Use "
            COLOR_ALT_TEXT_1 "resize" COLOR_INFO " to enlarge the palette if needed." COLOR_EOL,
            theme->palette_max - 1);
        return false;
    }

    bool color_is_num = is_number(argument);
    int idx = color_is_num ? atoi(argument) : -1;

    Color color = { 0 };
    if (theme->mode == COLOR_MODE_16) {
        if (!color_is_num) {
            if (!lookup_color(argument, &color, ch))
                return false;
        }
        else if (idx < 0 || idx > 16) {
            send_to_char(COLOR_INFO "Choose a color index from 0 to 15." COLOR_EOL, ch);
            return false;
        }
        else
            set_color_ansi(&color, (uint8_t)idx / 8, (uint8_t)idx % 8);
    }
    else if (theme->mode == COLOR_MODE_256) {
        if (!color_is_num) {
            if (!lookup_color(argument, &color, ch))
                return false;
        }
        else if (idx < 0 || idx > 255) {
            send_to_char(COLOR_INFO "Choose a color index from 0 to 255." COLOR_EOL, ch);
            send_256_color_list(ch);
            return false;
        }
        else
            set_color_256(&color, (uint8_t)idx);
    }
    else if (theme->mode == COLOR_MODE_RGB) {
        if (!lookup_color(argument, &color, ch))
            return false;
    }
    else {
        send_to_char(COLOR_INFO "Theme palettes cannot refer to themselves. Choose a color." COLOR_EOL, ch);
        return false;
    }

    free_string(theme->palette[pal_idx].cache);
    free_string(theme->palette[pal_idx].xterm);
    theme->palette[pal_idx] = color;
    theme->is_changed = true;
    send_to_char(COLOR_INFO "Palette color updated." COLOR_EOL, ch);
    return true;
}

THEMEEDIT(theme_edit_resize)
{
    ColorTheme* theme;
    if (!theme_edit_check(ch, &theme))
        return false;

    if (IS_NULLSTR(argument) || !is_number(argument)) {
        send_to_char(COLOR_INFO "USAGE: resize <palette size>" COLOR_EOL, ch);
        return false;
    }

    int new_size = atoi(argument);
    if (new_size < 1 || new_size > PALETTE_SIZE) {
        printf_to_char(ch, COLOR_INFO "Palette size must be between 1 and %d." COLOR_EOL,
            PALETTE_SIZE);
        return false;
    }

    if (new_size == theme->palette_max) {
        send_to_char(COLOR_INFO "The palette is already that size." COLOR_EOL, ch);
        return false;
    }

    Color default_color = theme->palette_max > 0 ? theme->palette[0] : (Color){
        .mode = theme->mode == COLOR_MODE_16 ? COLOR_MODE_16 :
                (theme->mode == COLOR_MODE_256 ? COLOR_MODE_256 : COLOR_MODE_RGB),
        .code = { 0, 0, 0 },
        .cache = NULL,
        .xterm = NULL
    };

    if (new_size < theme->palette_max) {
        for (int i = new_size; i < theme->palette_max; ++i) {
            free_string(theme->palette[i].cache);
            theme->palette[i].cache = NULL;
            free_string(theme->palette[i].xterm);
            theme->palette[i].xterm = NULL;
            theme->palette[i] = default_color;
        }
    }
    else {
        for (int i = theme->palette_max; i < new_size; ++i) {
            theme->palette[i] = default_color;
            theme->palette[i].cache = NULL;
            theme->palette[i].xterm = NULL;
        }
    }

    theme->palette_max = new_size;
    theme->is_changed = true;
    printf_to_char(ch, COLOR_INFO "Palette resized to %d entries." COLOR_EOL, new_size);
    return true;
}

THEMEEDIT(theme_edit_show_cmd)
{
    ColorTheme* theme;
    if (!theme_edit_check(ch, &theme))
        return false;

    theme_show_theme(ch, theme);
    return false;
}

THEMEEDIT(theme_edit_preview_cmd)
{
    ColorTheme* theme;
    if (!theme_edit_check(ch, &theme))
        return false;

    theme_preview_theme(ch, theme);
    return false;
}

THEMEEDIT(theme_edit_save)
{
    (void)argument;
    if (!theme_save_personal(ch))
        return false;

    send_to_char(COLOR_INFO "Theme saved." COLOR_EOL, ch);
    return false;
}

THEMEEDIT(theme_edit_discard)
{
    (void)argument;
    if (!theme_discard_personal(ch))
        return false;

    send_to_char(COLOR_INFO "Reverted to the saved version of this theme." COLOR_EOL, ch);
    theme_show_theme(ch, ch->pcdata->current_theme);
    return false;
}
