////////////////////////////////////////////////////////////////////////////////
// persist/theme/rom-olc/theme_persist_rom_olc.c
////////////////////////////////////////////////////////////////////////////////

#include "theme_persist_rom_olc.h"

#include "theme_rom_olc_io.h"

#include <color.h>

#include <persist/persist_io_adapters.h>

#include <comm.h>
#include <db.h>

#include <stdlib.h>

const ThemePersistFormat THEME_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = theme_persist_rom_olc_load,
    .save = theme_persist_rom_olc_save,
};

static void free_loaded_themes(ColorTheme** themes, int count)
{
    if (!themes)
        return;
    for (int i = 0; i < count; ++i) {
        if (!themes[i])
            continue;
        themes[i]->type = COLOR_THEME_TYPE_CUSTOM;
        free_color_theme(themes[i]);
    }
    free(themes);
}

PersistResult theme_persist_rom_olc_load(const PersistReader* reader, const char* filename)
{
    if (!reader || reader->ops != &PERSIST_FILE_STREAM_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "theme ROM load requires FILE reader", -1 };

    FILE* fp = (FILE*)reader->ctx;
    int count = fread_number(fp);
    if (count < 0)
        return (PersistResult){ PERSIST_ERR_FORMAT, "theme ROM load: invalid count", -1 };

    ColorTheme** themes = NULL;
    if (count > 0) {
        themes = calloc((size_t)count, sizeof(ColorTheme*));
        if (!themes)
            return (PersistResult){ PERSIST_ERR_INTERNAL, "theme ROM load: alloc failed", -1 };
    }

    for (int i = 0; i < count; ++i) {
        char* word = fread_word(fp);
        if (str_cmp(word, "#THEME")) {
            free_loaded_themes(themes, count);
            return (PersistResult){ PERSIST_ERR_FORMAT, "theme ROM load: missing #THEME", -1 };
        }

        ColorTheme* theme = theme_rom_olc_read_theme(fp, "#END", filename);
        if (!theme) {
            free_loaded_themes(themes, count);
            return (PersistResult){ PERSIST_ERR_INTERNAL, "theme ROM load: failed to read theme", -1 };
        }

        theme->type = COLOR_THEME_TYPE_SYSTEM;
        theme->is_public = false;
        theme->is_changed = false;
        themes[i] = theme;
    }

    if (!color_register_system_themes(themes, count)) {
        free_loaded_themes(themes, count);
        return (PersistResult){ PERSIST_ERR_INTERNAL, "theme ROM load: failed to register themes", -1 };
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult theme_persist_rom_olc_save(const PersistWriter* writer, const char* filename)
{
    if (!writer || writer->ops != &PERSIST_FILE_WRITER_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "theme ROM save requires FILE writer", -1 };

    FILE* fp = (FILE*)writer->ctx;
    if (!system_color_themes || system_color_theme_count <= 0)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "no system themes loaded", -1 };

    int total = 0;
    for (int i = 0; i < system_color_theme_count; ++i) {
        if (system_color_themes[i])
            ++total;
    }

    if (total == 0)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "no system themes available to save", -1 };

    fprintf(fp, "%d\n\n", total);

    for (int i = 0; i < system_color_theme_count; ++i) {
        const ColorTheme* theme = system_color_themes[i];
        if (!theme)
            continue;

        ColorTheme temp = *theme;
        temp.type = COLOR_THEME_TYPE_SYSTEM;
        temp.is_public = false;
        temp.is_changed = false;
        theme_rom_olc_write_theme(fp, &temp, "#END");
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
