////////////////////////////////////////////////////////////////////////////////
// persist/theme/rom-olc/theme_rom_olc_io.c
////////////////////////////////////////////////////////////////////////////////

#include "theme_rom_olc_io.h"

#include <comm.h>
#include <db.h>

#include <stdbool.h>
#include <stdlib.h>

static const char* theme_owner_label(const char* owner)
{
    return owner && owner[0] ? owner : "theme";
}

ColorTheme* theme_rom_olc_read_theme(FILE* fp, const char* end_token, const char* owner)
{
    if (!fp || !end_token)
        return NULL;

    ColorTheme* theme = new_color_theme();
    if (!theme)
        return NULL;

    while (true) {
        char* word = feof(fp) ? (char*)end_token : fread_word(fp);
        bool match = false;

        if (!str_cmp(word, end_token))
            break;

        switch (UPPER(word[0])) {
        case 'B':
            if (!str_cmp(word, "Banner")) {
                free_string(theme->banner);
                theme->banner = fread_string(fp);
                match = true;
            }
            break;
        case 'C':
            if (!str_cmp(word, "Channel")) {
                char* chan = fread_word(fp);
                int mode = fread_number(fp);
                uint8_t code_0 = (uint8_t)fread_number(fp);
                uint8_t code_1 = (uint8_t)fread_number(fp);
                uint8_t code_2 = (uint8_t)fread_number(fp);
                fread_number(fp); // reserved
                int slot = -1;
                LOOKUP_COLOR_SLOT_NAME(slot, chan);
                if (slot < 0 || slot >= COLOR_SLOT_COUNT) {
                    bugf("theme_rom_olc_read(%s): bad channel '%s'.",
                        theme_owner_label(owner), chan);
                }
                else {
                    theme->channels[slot] = (Color){
                        .mode = mode,
                        .code = { code_0, code_1, code_2 },
                        .cache = NULL,
                        .xterm = NULL,
                    };
                }
                match = true;
            }
            break;
        case 'I':
            if (!str_cmp(word, "Info")) {
                theme->type = fread_number(fp);
                theme->mode = fread_number(fp);
                theme->palette_max = fread_number(fp);
                theme->is_public = fread_number(fp);
                if (theme->palette_max < 0)
                    theme->palette_max = 0;
                if (theme->palette_max > PALETTE_SIZE)
                    theme->palette_max = PALETTE_SIZE;
                match = true;
            }
            break;
        case 'N':
            if (!str_cmp(word, "Name")) {
                free_string(theme->name);
                theme->name = fread_string(fp);
                match = true;
            }
            break;
        case 'P':
            if (!str_cmp(word, "Palette")) {
                int idx = fread_number(fp);
                int mode = fread_number(fp);
                uint8_t code_0 = (uint8_t)fread_number(fp);
                uint8_t code_1 = (uint8_t)fread_number(fp);
                uint8_t code_2 = (uint8_t)fread_number(fp);
                fread_number(fp); // reserved
                if (idx < 0 || idx >= PALETTE_SIZE) {
                    bugf("theme_rom_olc_read(%s): bad palette index %d.",
                        theme_owner_label(owner), idx);
                }
                else {
                    theme->palette[idx] = (Color){
                        .mode = mode,
                        .code = { code_0, code_1, code_2 },
                        .cache = NULL,
                        .xterm = NULL,
                    };
                }
                match = true;
            }
            break;
        default:
            break;
        }

        if (!match) {
            bugf("theme_rom_olc_read(%s): no match for '%s'.",
                theme_owner_label(owner), word);
            fread_to_eol(fp);
        }
    }

    theme->is_changed = false;
    return theme;
}

void theme_rom_olc_write_theme(FILE* fp, const ColorTheme* theme, const char* end_token)
{
    if (!fp || !theme || !end_token)
        return;

    fprintf(fp, "#THEME\n");
    fprintf(fp, "Name %s~\n", theme->name ? theme->name : "");
    fprintf(fp, "Banner %s~\n", theme->banner ? theme->banner : "");
    fprintf(fp, "Info %d %d %d %d\n",
        theme->type,
        theme->mode,
        UMIN(theme->palette_max, PALETTE_SIZE),
        theme->is_public ? 1 : 0);

    int pal_max = UMIN(theme->palette_max, PALETTE_SIZE);
    for (int i = 0; i < pal_max; ++i) {
        const Color* color = &theme->palette[i];
        fprintf(fp, "Palette %d %u %u %u %u 0\n",
            i,
            color->mode,
            color->code[0],
            color->code[1],
            color->code[2]);
    }

    for (int i = 0; i < COLOR_SLOT_COUNT; ++i) {
        const Color* color = &theme->channels[i];
        fprintf(fp, "Channel %s %u %u %u %u 0\n",
            color_slot_entries[i].name,
            color->mode,
            color->code[0],
            color->code[1],
            color->code[2]);
    }

    fprintf(fp, "%s\n\n", end_token);
}
