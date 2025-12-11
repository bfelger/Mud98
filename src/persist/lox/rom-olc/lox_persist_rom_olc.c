////////////////////////////////////////////////////////////////////////////////
// persist/lox/rom-olc/lox_persist_rom_olc.c
////////////////////////////////////////////////////////////////////////////////

#include "lox_persist_rom_olc.h"

#include <persist/persist_io_adapters.h>

#include <lox/lox.h>

#include <comm.h>
#include <db.h>

#include <stdio.h>
#include <string.h>

const LoxPersistFormat LOX_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = lox_persist_rom_olc_load,
    .save = lox_persist_rom_olc_save,
};

static PersistResult load_single_entry(FILE* fp)
{
    char* category = NULL;
    char* file = NULL;
    LoxScriptWhen when = LOX_SCRIPT_WHEN_PRE;
    bool done = false;

    while (!done) {
        char* word = fread_word(fp);
        if (!str_cmp(word, "#END")) {
            done = true;
            break;
        }

        if (!str_cmp(word, "category")) {
            category = fread_string(fp);
        }
        else if (!str_cmp(word, "file")) {
            file = fread_string(fp);
        }
        else if (!str_cmp(word, "when")) {
            char* when_val = fread_string(fp);
            if (!lox_script_when_parse(when_val, &when)) {
                bugf("lox ROM load: unknown when '%s'", when_val);
                return (PersistResult){ PERSIST_ERR_FORMAT, "lox ROM load: bad when value", -1 };
            }
        }
        else {
            bugf("lox ROM load: unknown key '%s'", word);
            fread_to_eol(fp);
        }
    }

    if (!file || file[0] == '\0') {
        bug("lox ROM load: script missing file name", 0);
        return (PersistResult){ PERSIST_ERR_FORMAT, "lox ROM load: script missing file", -1 };
    }

    LoxScriptEntry* entry = lox_script_entry_append_loaded(category ? category : "", file, when);
    if (!entry)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "lox ROM load: allocation failed", -1 };

    (void)entry;
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult lox_persist_rom_olc_load(const PersistReader* reader, const char* filename)
{
    if (!reader || reader->ops != &PERSIST_FILE_STREAM_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "lox ROM load requires FILE reader", -1 };

    FILE* fp = (FILE*)reader->ctx;
    int script_count = fread_number(fp);

    for (int i = 0; i < script_count; ++i) {
        char* section = fread_word(fp);
        if (str_cmp(section, "#LOX_SCRIPT")) {
            bugf("lox ROM load: expected #LOX_SCRIPT, found %s", section);
            return (PersistResult){ PERSIST_ERR_FORMAT, "lox ROM load: bad section header", -1 };
        }

        PersistResult res = load_single_entry(fp);
        if (!persist_succeeded(res))
            return res;
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult lox_persist_rom_olc_save(const PersistWriter* writer, const char* filename)
{
    if (!writer || writer->ops != &PERSIST_FILE_WRITER_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "lox ROM save requires FILE writer", -1 };

    FILE* fp = (FILE*)writer->ctx;
    size_t count = lox_script_entry_count();

    fprintf(fp, "%zu\n\n", count);
    for (size_t i = 0; i < count; ++i) {
        const LoxScriptEntry* entry = lox_script_entry_get(i);
        if (!entry)
            continue;

        const char* category = entry->category ? entry->category : "";
        const char* file = entry->file ? entry->file : "";
        const char* when = lox_script_when_name(entry->when);

        fprintf(fp, "#LOX_SCRIPT\n");
        fprintf(fp, "category %s~\n", category);
        fprintf(fp, "file %s~\n", file);
        fprintf(fp, "when %s~\n", when);
        fprintf(fp, "#END\n\n");
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
