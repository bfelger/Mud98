////////////////////////////////////////////////////////////////////////////////
// persist/command/rom-olc/command_persist_rom_olc.c
////////////////////////////////////////////////////////////////////////////////

#include "command_persist_rom_olc.h"

#include <persist/persist_io_adapters.h>

#include <persist/command/command_persist.h>

#include <tablesave.h>

#include <comm.h>
#include <db.h>
#include <interp.h>
#include <lookup.h>

#include <data/mobile_data.h>

#include <tables.h>

#include <stdlib.h>

#ifdef U
#undef U
#endif
#define U(x) (uintptr_t)(x)

extern bool test_output_enabled;

static CmdInfo tmp_cmd;

static char* do_fun_str(void* temp);
static bool do_fun_read(void* temp, char* arg);

static const SaveTableEntry cmdsavetable[] = {
    { "name",        FIELD_STRING,                U(&tmp_cmd.name),        0,                 0 },
    { "do_fun",      FIELD_FUNCTION_INT_TO_STR,   U(&tmp_cmd.do_fun),      U(do_fun_str),     U(do_fun_read) },
    { "position",    FIELD_FUNCTION_INT16_TO_STR, U(&tmp_cmd.position),    U(position_str),   U(position_read) },
    { "level",       FIELD_INT16,                 U(&tmp_cmd.level),       0,                 0 },
    { "log",         FIELD_INT16_FLAGSTRING,      U(&tmp_cmd.log),         U(log_flag_table), 0 },
    { "show",        FIELD_INT16_FLAGSTRING,      U(&tmp_cmd.show),        U(show_flag_table),0 },
    { NULL,          0,                           0,                       0,                 0 }
};

const CommandPersistFormat COMMAND_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = command_persist_rom_olc_load,
    .save = command_persist_rom_olc_save,
};

static char* do_fun_str(void* temp)
{
    DoFunc** fun = (DoFunc**)temp;
    return cmd_func_name(*fun);
}

static bool do_fun_read(void* temp, char* arg)
{
    DoFunc** fun = (DoFunc**)temp;
    *fun = cmd_func_lookup(arg);
    return true;
}

PersistResult command_persist_rom_olc_load(const PersistReader* reader, const char* filename)
{
    if (!reader || reader->ops != &PERSIST_FILE_STREAM_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "command ROM load requires FILE reader", -1 };

    FILE* fp = (FILE*)reader->ctx;
    extern CmdInfo* cmd_table;
    extern int max_cmd;
    char* word;

    int size = fread_number(fp);
    if (cmd_table)
        free(cmd_table);
    max_cmd = size;

    if (!test_output_enabled)
        printf_log("Creating cmd_table of length %d, size %zu", size + 1,
            sizeof(CmdInfo) * ((size_t)size + 1));

    cmd_table = calloc((size_t)size + 1, sizeof(CmdInfo));
    if (!cmd_table)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "command ROM load: alloc failed", -1 };

    for (int i = 0; i < size; ++i) {
        word = fread_word(fp);
        if (str_cmp(word, "#COMMAND")) {
            bugf("load_command_table : word %s", word);
            return (PersistResult){ PERSIST_ERR_FORMAT, "command ROM load: bad section", -1 };
        }
        load_struct(fp, U(&tmp_cmd), cmdsavetable, U(&cmd_table[i]));
    }

    cmd_table[size].name = str_dup("");
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult command_persist_rom_olc_save(const PersistWriter* writer, const char* filename)
{
    if (!writer || writer->ops != &PERSIST_FILE_WRITER_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "command ROM save requires FILE writer", -1 };

    FILE* fp = (FILE*)writer->ctx;
    extern CmdInfo* cmd_table;

    int count = 0;
    for (const CmdInfo* temp = cmd_table; temp && !IS_NULLSTR(temp->name); ++temp)
        ++count;

    fprintf(fp, "%d\n\n", count);

    for (const CmdInfo* temp = cmd_table; temp && !IS_NULLSTR(temp->name); ++temp) {
        fprintf(fp, "#COMMAND\n");
        save_struct(fp, U(&tmp_cmd), cmdsavetable, U(temp));
        fprintf(fp, "#END\n\n");
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

#ifdef U
#undef U
#endif
