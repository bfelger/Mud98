////////////////////////////////////////////////////////////////////////////////
// persist/command/json/command_persist_json.c
////////////////////////////////////////////////////////////////////////////////

#include "command_persist_json.h"

#include <persist/json/persist_json.h>

#include <comm.h>
#include <db.h>
#include <interp.h>
#include <lookup.h>

#include <data/mobile_data.h>

#include <tables.h>

#include <jansson.h>

#include <stdlib.h>
#include <string.h>

const CommandPersistFormat COMMAND_PERSIST_JSON = {
    .name = "json",
    .load = command_persist_json_load,
    .save = command_persist_json_save,
};

static const char* flag_name_from_value(FLAGS value, const struct flag_type* table)
{
    if (!table)
        return NULL;
    for (int i = 0; table[i].name != NULL; ++i) {
        if (table[i].bit == value)
            return table[i].name;
    }
    return NULL;
}

static FLAGS flag_value_from_name(const char* name, const struct flag_type* table, FLAGS def)
{
    if (!name)
        return def;
    FLAGS bit = flag_lookup(name, table);
    if (bit == NO_FLAG)
        return def;
    return bit;
}

static const char* position_name_safe(Position pos)
{
    if (pos < 0 || pos >= POS_MAX)
        return position_table[POS_DEAD].name;
    return position_table[pos].name;
}

static DoFunc* default_command_function(void)
{
    static DoFunc* fallback = NULL;
    if (!fallback) {
        char fallback_name[] = "do_nothing";
        fallback = cmd_func_lookup(fallback_name);
    }
    return fallback;
}

PersistResult command_persist_json_save(const PersistWriter* writer, const char* filename)
{
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "command JSON save: missing writer", -1 };

    extern CmdInfo* cmd_table;

    json_t* root = json_object();
    JSON_SET_INT(root, "formatVersion", 1);
    json_t* commands = json_array();

    for (int i = 0; cmd_table && !IS_NULLSTR(cmd_table[i].name); ++i) {
        CmdInfo* cmd = &cmd_table[i];
        if (IS_NULLSTR(cmd->name))
            continue;

        json_t* obj = json_object();
        JSON_SET_STRING(obj, "name", cmd->name);

        const char* func = cmd_func_name(cmd->do_fun);
        if (func && func[0] != '\0')
            JSON_SET_STRING(obj, "function", func);

        JSON_SET_STRING(obj, "position", position_name_safe(cmd->position));
        JSON_SET_INT(obj, "level", cmd->level);

        const char* log_name = flag_name_from_value(cmd->log, log_flag_table);
        if (log_name)
            JSON_SET_STRING(obj, "log", log_name);

        const char* show_name = flag_name_from_value(cmd->show, show_flag_table);
        if (show_name)
            JSON_SET_STRING(obj, "category", show_name);

        json_array_append_new(commands, obj);
    }

    json_object_set_new(root, "commands", commands);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "command JSON save: dump failed", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "command JSON save: write failed", -1 };
    if (writer->ops->flush)
        writer->ops->flush(writer->ctx);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult command_persist_json_load(const PersistReader* reader, const char* filename)
{
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "command JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "command JSON load: failed to read", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof(msg), "command JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }

    json_t* fmtv = json_object_get(root, "formatVersion");
    if (json_is_integer(fmtv) && json_integer_value(fmtv) != 1) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "command JSON: unsupported formatVersion", -1 };
    }

    json_t* commands = json_object_get(root, "commands");
    if (!json_is_array(commands)) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "command JSON: missing commands array", -1 };
    }

    size_t count = json_array_size(commands);
    extern CmdInfo* cmd_table;
    extern int max_cmd;

    if (cmd_table)
        free(cmd_table);
    cmd_table = calloc(count + 1, sizeof(CmdInfo));
    if (!cmd_table) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_INTERNAL, "command JSON: alloc failed", -1 };
    }

    for (size_t i = 0; i < count; ++i) {
        json_t* cmd_obj = json_array_get(commands, i);
        if (!json_is_object(cmd_obj))
            continue;
        CmdInfo* cmd = &cmd_table[i];

        const char* name = JSON_STRING(cmd_obj, "name");
        if (name)
            cmd->name = boot_intern_string(name);

        const char* func = JSON_STRING(cmd_obj, "function");
        if (func && func[0] != '\0') {
            cmd->do_fun = cmd_func_lookup((char*)func);
        }
        if (!cmd->do_fun)
            cmd->do_fun = default_command_function();

        const char* pos_name = JSON_STRING(cmd_obj, "position");
        if (pos_name)
            cmd->position = position_lookup(pos_name);
        else
            cmd->position = POS_DEAD;

        json_t* level_val = json_object_get(cmd_obj, "level");
        if (json_is_integer(level_val))
            cmd->level = (int16_t)json_integer_value(level_val);

        const char* log_name = JSON_STRING(cmd_obj, "log");
        cmd->log = (int16_t)flag_value_from_name(log_name, log_flag_table, LOG_NORMAL);

        const char* cat_name = JSON_STRING(cmd_obj, "category");
        cmd->show = (int16_t)flag_value_from_name(cat_name, show_flag_table, TYP_UNDEF);
    }

    cmd_table[count].name = str_dup("");
    max_cmd = (int)count;
    json_decref(root);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
