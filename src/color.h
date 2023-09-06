////////////////////////////////////////////////////////////////////////////////
// color.h
// Functions to handle VT102 SGR colors
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__COLOR_H
#define MUD98__COLOR_H

#include "merc.h"

#include "color.h"
#include "vt.h"

#include <stdint.h>

typedef enum {
    COLOR_MODE_16       = 0,    // 16-color
    COLOR_MODE_256      = 1,    // Indexed 256-color
    COLOR_MODE_RGB      = 2,    // 24-bit color
    COLOR_MODE_PAL_IDX  = 3,    // Indexed against theme palette
} ColorMode;

typedef struct {
    ColorMode mode;
    uint8_t code[3];
    char* cache;
    char* xterm;
} Color;

typedef enum {
    COLOR_THEME_TYPE_CUSTOM,
    COLOR_THEME_TYPE_SYSTEM_COPY,
    COLOR_THEME_TYPE_SYSTEM,
} ColorThemeType;

#define PAL_WHITE      0
#define PAL_BLACK      1
#define PAL_D_GRAY     2
#define PAL_B_WHITE    3
#define PAL_RED        4
#define PAL_B_RED      5
#define PAL_GREEN      6
#define PAL_B_GREEN    7
#define PAL_YELLOW     8
#define PAL_B_YELLOW   9
#define PAL_BLUE       10
#define PAL_B_BLUE     11
#define PAL_MAGENTA    12
#define PAL_B_MAGENTA  13
#define PAL_CYAN       14
#define PAL_B_CYAN     15

typedef enum {
    SLOT_TEXT,                  // {t
    SLOT_BACKGROUND,            // {X
    SLOT_TITLE,                 // {#
    SLOT_ALT_TEXT_1,            // {*
    SLOT_ALT_TEXT_2,            // {_
    SLOT_DECOR_1,               // {|
    SLOT_DECOR_2,               // {/
    SLOT_DECOR_3,               // {backwack
    SLOT_AUCTION,               // {a
    SLOT_AUCTION_TEXT,          // {A
    SLOT_GOSSIP,                // {d
    SLOT_GOSSIP_TEXT,           // {9
    SLOT_MUSIC,                 // {e
    SLOT_MUSIC_TEXT,            // {E
    SLOT_QUESTION,              // {q
    SLOT_QUESTION_TEXT,         // {Q
    SLOT_ANSWER,                // {f
    SLOT_ANSWER_TEXT,           // {F
    SLOT_QUOTE,                 // {h
    SLOT_QUOTE_TEXT,            // {H
    SLOT_IMMTALK_TEXT,          // {i
    SLOT_IMMTALK_TYPE,          // {I
    SLOT_INFO,                  // {j
    SLOT_SAY,                   // {6
    SLOT_SAY_TEXT,              // {7
    SLOT_TELL,                  // {k
    SLOT_TELL_TEXT,             // {K
    SLOT_REPLY,                 // {l
    SLOT_REPLY_TEXT,            // {L
    SLOT_GTELL_TEXT,            // {n
    SLOT_GTELL_TYPE,            // {N
    SLOT_WIZNET,                // {B
    SLOT_ROOM_TITLE,            // {s
    SLOT_ROOM_TEXT,             // {S
    SLOT_ROOM_EXITS,            // {o
    SLOT_ROOM_THINGS,           // {O
    SLOT_PROMPT,                // {p
    SLOT_FIGHT_DEATH,           // {1
    SLOT_FIGHT_YHIT,            // {2
    SLOT_FIGHT_OHIT,            // {3
    SLOT_FIGHT_THIT,            // {4
    SLOT_FIGHT_SKILL,           // {5
    SLOT_MAX
} ColorChannel;

typedef struct color_slot_entry_t {
    const char* name;
    const ColorChannel channel;
    const char code;
} ColorChannelEntry;

// Always pad out to 16, using SLOT 0 for extras if you must. Code and builders
// should be able to depend on being able to reference up to 16 color slots.
#define PALETTE_SIZE        16

typedef struct color_theme_t {
    char* name;             // Keyword used to select the theme.
    ColorThemeType type;    // Remember if it's a system theme, so we can alter
                            //      the name for user copies to indicate that
                            //      it's a modified, custom version.
    ColorMode mode;
    bool is_public;
    bool is_changed;
    char* banner;           // Display after theme-change. Use this to prompt
                            //      a manual background color-change in case 
                            //      client doesn't respect OSC, or use this 
                            //      space for credits.
    Color palette[PALETTE_SIZE];
    int palette_max;
    Color channels[SLOT_MAX];
} ColorTheme;

typedef enum {
    SYSTEM_COLOR_THEME_LOPE,
    SYSTEM_COLOR_THEME_MONO,
    SYSTEM_COLOR_THEME_COOL_CAT,
    SYSTEM_COLOR_THEME_CITY,
    SYSTEM_COLOR_THEME_POP,
    SYSTEM_COLOR_THEME_MAX
} SystemColorTheme;

typedef struct ansi_palette_entry_t {
    const char* name;
    int bright;
    int code;
    Color color;
} AnsiPaletteEntry;

extern const AnsiPaletteEntry ansi_palette[];
extern const ColorTheme* system_color_themes[];
extern const ColorChannelEntry color_slot_entries[];

char* bg_color_to_str(const ColorTheme* theme, const Color* color, bool xterm);
char* color_to_str(ColorTheme* theme, Color* color, bool xterm);
ColorTheme* dup_color_theme(const ColorTheme* theme);
void free_color_theme(ColorTheme* theme);
ColorMode lookup_color_mode(const char* arg);
ColorTheme* lookup_color_theme(CHAR_DATA* ch, char* arg);
ColorTheme* lookup_local_color_theme(CHAR_DATA* ch, char* arg);
ColorTheme* lookup_remote_color_theme(CHAR_DATA* ch, char* arg);
ColorTheme* lookup_system_color_theme(CHAR_DATA* ch, char* arg);
ColorTheme* lookup_local_color_theme(CHAR_DATA* ch, char* arg);
ColorTheme* new_color_theme();
void set_color_ansi(Color* color, uint8_t light, uint8_t index);
void set_color_256(Color* color, uint8_t index);
void set_color_palette_ref(Color* color, uint8_t index);
void set_color_rgb(Color* color, uint8_t r, uint8_t g, uint8_t b);
void set_default_colors(CHAR_DATA* ch);

#define LOOKUP_COLOR_SLOT_CODE(s, c)                                        \
    for (int i_c = 0; i_c < SLOT_MAX; ++i_c)                                \
        if (c == color_slot_entries[i_c].code) {                            \
            s = color_slot_entries[i_c].channel;                            \
            break;                                                          \
        }

#define LOOKUP_COLOR_SLOT_NAME(s, n)                                        \
    for (int i_c = 0; i_c < SLOT_MAX; ++i_c)                                \
        if (!str_cmp(n, color_slot_entries[i_c].name)) {                    \
            s = color_slot_entries[i_c].channel;                            \
            break;                                                          \
        }                                                                   \

#define LOOKUP_PALETTE_CODE(pal, c)                                         \
switch (type) {                                                             \
case 'b': pal = PAL_BLUE; break;                                            \
case 'c': pal = PAL_GREEN; break;                                           \
case 'g': pal = PAL_GREEN; break;                                           \
case 'm': pal = PAL_MAGENTA; break;                                         \
case 'r': pal = PAL_RED; break;                                             \
case 'w': pal = PAL_WHITE; break;                                           \
case 'y': pal = PAL_YELLOW; break;                                          \
case 'B': pal = PAL_B_BLUE; break;                                          \
case 'C': pal = PAL_B_CYAN; break;                                          \
case 'G': pal = PAL_B_GREEN; break;                                         \
case 'M': pal = PAL_B_MAGENTA; break;                                       \
case 'R': pal = PAL_B_RED; break;                                           \
case 'W': pal = PAL_B_WHITE; break;                                         \
case 'Y': pal = PAL_B_YELLOW; break;                                        \
case 'D': pal = PAL_D_GRAY; break;                                          \
default: break;                                                             \
}

#endif // !MUD98__COLOR_H