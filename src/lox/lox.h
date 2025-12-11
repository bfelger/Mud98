////////////////////////////////////////////////////////////////////////////////
// lox.h
// Intended to be the one header to include if you are calling Lox code from
// native C.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef LOX__LOX_H
#define LOX__LOX_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct entity_t Entity;
typedef struct room_t Room;
typedef struct mobile_t Mobile;
typedef struct object_t Object;

#include "array.h"
#include "function.h"
#include "list.h"
#include "object.h"
#include "table.h"
#include "value.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

typedef struct {
    Mobile* me;
    Entity* this_;
} CompileContext;

typedef struct {
    Mobile* me;
    bool is_repl;
} ExecContext;

typedef ObjString String;

void add_global(const char* name, Value val);
InterpretResult call_function(const char* fn_name, int count, ...);
InterpretResult invoke_closure(ObjClosure* closure, int count, ...);
ObjClass* find_class(const char* class_name);
void free_vm();
void init_const_natives();
void init_world_natives();
void init_vm();
InterpretResult interpret_code(const char* source);
Value pop();
void push(Value value);
void runtime_error(const char* format, ...);

bool lox_streq(ObjString* a, ObjString* b);

extern char str_empty[1];
extern ObjString* lox_empty_string;

extern CompileContext compile_context;
extern ExecContext exec_context;

extern Value repl_ret_val; // The final result of REPL and in-line code

typedef enum {
    LOX_SCRIPT_WHEN_PRE,
    LOX_SCRIPT_WHEN_POST,
} LoxScriptWhen;

typedef struct lox_script_entry_t {
    char* category;
    char* file;
    LoxScriptWhen when;
    char* source;
    bool has_source;
    bool executed;
    bool script_dirty;
} LoxScriptEntry;

void load_lox_public_scripts(void);
void run_post_lox_public_scripts(void);
void save_lox_public_scripts(bool force_catalog);
void save_lox_public_scripts_if_dirty(void);
void lox_script_registry_clear(void);

size_t lox_script_entry_count(void);
LoxScriptEntry* lox_script_entry_get(size_t index);
LoxScriptEntry* lox_script_entry_create(const char* category, const char* file, LoxScriptWhen when);
LoxScriptEntry* lox_script_entry_append_loaded(const char* category, const char* file, LoxScriptWhen when);
bool lox_script_entry_delete(size_t index);
bool lox_script_entry_set_category(LoxScriptEntry* entry, const char* category);
bool lox_script_entry_set_file(LoxScriptEntry* entry, const char* file);
bool lox_script_entry_set_when(LoxScriptEntry* entry, LoxScriptWhen when);
bool lox_script_entry_update_source(LoxScriptEntry* entry, const char* text);
bool lox_script_entry_ensure_source(LoxScriptEntry* entry);
bool lox_script_entry_execute(LoxScriptEntry* entry);
int lox_script_entry_index(const LoxScriptEntry* entry);
const char* lox_script_when_name(LoxScriptWhen when);
bool lox_script_when_parse(const char* text, LoxScriptWhen* when_out);
void lox_script_entry_source_path(char* out, size_t out_len, const LoxScriptEntry* entry);

#endif // !LOX__LOX_H
