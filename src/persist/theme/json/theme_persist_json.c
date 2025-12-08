////////////////////////////////////////////////////////////////////////////////
// persist/theme/json/theme_persist_json.c
////////////////////////////////////////////////////////////////////////////////

#include "theme_persist_json.h"

#include <persist/json/persist_json.h>

#include <color.h>

#include <persist/persist_io_adapters.h>

#include <comm.h>
#include <db.h>

#include <stdlib.h>
#include <string.h>

const ThemePersistFormat THEME_PERSIST_JSON = {
    .name = "json",
    .load = theme_persist_json_load,
    .save = theme_persist_json_save,
};

static const char* color_mode_to_string(ColorMode mode)
{
    switch (mode) {
    case COLOR_MODE_16: return "ansi";
    case COLOR_MODE_256: return "256";
    case COLOR_MODE_RGB: return "rgb";
    case COLOR_MODE_PAL_IDX: return "palette";
    default: return NULL;
    }
}

static bool color_mode_from_string(const char* name, ColorMode* out_mode)
{
    if (!name || !out_mode)
        return false;

    if (!str_cmp(name, "ansi"))
        *out_mode = COLOR_MODE_16;
    else if (!str_cmp(name, "256"))
        *out_mode = COLOR_MODE_256;
    else if (!str_cmp(name, "rgb"))
        *out_mode = COLOR_MODE_RGB;
    else if (!str_cmp(name, "palette"))
        *out_mode = COLOR_MODE_PAL_IDX;
    else
        return false;

    return true;
}

static int color_code_count(ColorMode mode)
{
    switch (mode) {
    case COLOR_MODE_RGB: return 3;
    case COLOR_MODE_16: return 2;
    default: return 1;
    }
}

static json_t* color_to_json(const Color* color)
{
    if (!color)
        return NULL;

    const char* mode_name = color_mode_to_string(color->mode);
    if (!mode_name)
        return NULL;

    json_t* obj = json_object();
    JSON_SET_STRING(obj, "mode", mode_name);
    json_t* code = json_array();
    int count = color_code_count(color->mode);
    for (int i = 0; i < count; ++i)
        json_array_append_new(code, json_integer(color->code[i]));
    json_object_set_new(obj, "code", code);
    return obj;
}

static bool color_from_json(json_t* obj, Color* out_color)
{
    if (!json_is_object(obj) || !out_color)
        return false;

    const char* mode_name = JSON_STRING(obj, "mode");
    ColorMode mode = COLOR_MODE_16;
    if (!color_mode_from_string(mode_name, &mode))
        return false;

    json_t* code = json_object_get(obj, "code");
    if (!json_is_array(code))
        return false;

    out_color->mode = mode;
    out_color->code[0] = out_color->code[1] = out_color->code[2] = 0;
    int expected = color_code_count(mode);
    size_t arr_size = json_array_size(code);
    for (int i = 0; i < expected && (size_t)i < arr_size; ++i) {
        json_t* val = json_array_get(code, (size_t)i);
        if (!json_is_integer(val))
            return false;
        out_color->code[i] = (uint8_t)json_integer_value(val);
    }
    out_color->cache = NULL;
    out_color->xterm = NULL;
    return true;
}

static const char* theme_type_to_string(ColorThemeType type)
{
    switch (type) {
    case COLOR_THEME_TYPE_SYSTEM: return "system";
    case COLOR_THEME_TYPE_SYSTEM_COPY: return "system_copy";
    case COLOR_THEME_TYPE_CUSTOM:
    default:
        return "custom";
    }
}

static ColorThemeType theme_type_from_string(const char* name)
{
    if (!name)
        return COLOR_THEME_TYPE_SYSTEM;
    if (!str_cmp(name, "system"))
        return COLOR_THEME_TYPE_SYSTEM;
    if (!str_cmp(name, "system_copy"))
        return COLOR_THEME_TYPE_SYSTEM_COPY;
    return COLOR_THEME_TYPE_CUSTOM;
}

static void free_theme_list(ColorTheme** themes, size_t count)
{
    if (!themes)
        return;
    for (size_t i = 0; i < count; ++i) {
        if (!themes[i])
            continue;
        themes[i]->type = COLOR_THEME_TYPE_CUSTOM;
        free_color_theme(themes[i]);
    }
    free(themes);
}

PersistResult theme_persist_json_save(const PersistWriter* writer, const char* filename)
{
    (void)filename;
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "theme JSON save: missing writer", -1 };

    if (!system_color_themes || system_color_theme_count <= 0)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "theme JSON save: no system themes loaded", -1 };

    json_t* root = json_object();
    JSON_SET_INT(root, "formatVersion", 1);
    json_t* themes = json_array();

    for (int i = 0; i < system_color_theme_count; ++i) {
        const ColorTheme* theme = system_color_themes[i];
        if (!theme)
            continue;

        json_t* obj = json_object();
        JSON_SET_STRING(obj, "name", theme->name ? theme->name : "");
        if (theme->banner && theme->banner[0])
            JSON_SET_STRING(obj, "banner", theme->banner);
        const char* mode_name = color_mode_to_string(theme->mode);
        if (mode_name)
            JSON_SET_STRING(obj, "mode", mode_name);
        JSON_SET_INT(obj, "paletteMax", UMIN(theme->palette_max, PALETTE_SIZE));
        JSON_SET_STRING(obj, "type", theme_type_to_string(theme->type));
        json_object_set_new(obj, "public", json_boolean(theme->is_public));

        json_t* palette = json_array();
        int pal_max = UMIN(theme->palette_max, PALETTE_SIZE);
        for (int idx = 0; idx < pal_max; ++idx) {
            json_t* entry = color_to_json(&theme->palette[idx]);
            if (!entry)
                continue;
            JSON_SET_INT(entry, "index", idx);
            json_array_append_new(palette, entry);
        }
        json_object_set_new(obj, "palette", palette);

        json_t* channels = json_object();
        for (int c = 0; c < COLOR_SLOT_COUNT; ++c) {
            json_t* chan = color_to_json(&theme->channels[c]);
            if (!chan)
                continue;
            json_object_set_new(channels, color_slot_entries[c].name, chan);
        }
        json_object_set_new(obj, "channels", channels);

        json_array_append_new(themes, obj);
    }

    json_object_set_new(root, "themes", themes);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "theme JSON save: serialization failed", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "theme JSON save: write failed", -1 };
    if (writer->ops->flush)
        writer->ops->flush(writer->ctx);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult theme_persist_json_load(const PersistReader* reader, const char* filename)
{
    (void)filename;
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "theme JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "theme JSON load: failed to read", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof(msg), "theme JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }

    json_t* fmt = json_object_get(root, "formatVersion");
    if (json_is_integer(fmt) && json_integer_value(fmt) != 1) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "theme JSON: unsupported formatVersion", -1 };
    }

    json_t* themes = json_object_get(root, "themes");
    if (!json_is_array(themes)) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "theme JSON: missing themes array", -1 };
    }

    size_t count = json_array_size(themes);
    if (count == 0) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "theme JSON: no themes defined", -1 };
    }

    ColorTheme** list = calloc(count, sizeof(ColorTheme*));
    if (!list) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_INTERNAL, "theme JSON: allocation failed", -1 };
    }

    for (size_t i = 0; i < count; ++i) {
        json_t* entry = json_array_get(themes, i);
        if (!json_is_object(entry))
            continue;

        ColorTheme* theme = new_color_theme();
        if (!theme) {
            free_theme_list(list, count);
            json_decref(root);
            return (PersistResult){ PERSIST_ERR_INTERNAL, "theme JSON: allocation failed", -1 };
        }

        const char* name = JSON_STRING(entry, "name");
        if (name && name[0]) {
            free_string(theme->name);
            theme->name = boot_intern_string(name);
        }
        const char* banner = JSON_STRING(entry, "banner");
        JSON_INTERN(banner, theme->banner)

        const char* mode_name = JSON_STRING(entry, "mode");
        ColorMode mode = theme->mode;
        if (color_mode_from_string(mode_name, &mode))
            theme->mode = mode;

        int pal_max = (int)json_int_or_default(entry, "paletteMax", theme->palette_max);
        theme->palette_max = URANGE(0, pal_max, PALETTE_SIZE);

        const char* type_name = JSON_STRING(entry, "type");
        theme->type = theme_type_from_string(type_name);
        theme->is_public = json_is_true(json_object_get(entry, "public"));

        json_t* palette = json_object_get(entry, "palette");
        if (json_is_array(palette)) {
            size_t pal_count = json_array_size(palette);
            for (size_t p = 0; p < pal_count; ++p) {
                json_t* pal_entry = json_array_get(palette, p);
                if (!json_is_object(pal_entry))
                    continue;
                int idx = (int)json_int_or_default(pal_entry, "index", -1);
                if (idx < 0 || idx >= PALETTE_SIZE)
                    continue;
                Color color = { 0 };
                if (!color_from_json(pal_entry, &color))
                    continue;
                theme->palette[idx] = color;
            }
        }

        json_t* channels = json_object_get(entry, "channels");
        if (json_is_object(channels)) {
            const char* key;
            json_t* value;
            json_object_foreach(channels, key, value) {
                int chan = -1;
                LOOKUP_COLOR_SLOT_NAME(chan, key);
                if (chan < 0 || chan >= COLOR_SLOT_COUNT)
                    continue;
                Color color = { 0 };
                if (!color_from_json(value, &color))
                    continue;
                theme->channels[chan] = color;
            }
        }

        theme->is_changed = false;
        theme->is_public = false;
        theme->type = COLOR_THEME_TYPE_SYSTEM;
        list[i] = theme;
    }

    PersistResult res = { PERSIST_OK, NULL, -1 };
    if (!color_register_system_themes(list, (int)count)) {
        free_theme_list(list, count);
        res = (PersistResult){ PERSIST_ERR_INTERNAL, "theme JSON: failed to register themes", -1 };
    }

    json_decref(root);
    return res;
}
