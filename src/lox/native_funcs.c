////////////////////////////////////////////////////////////////////////////////
// native_funcs.h
// Enumerate native functions for Lox and add them to global
////////////////////////////////////////////////////////////////////////////////

#include "native.h"

#include <data/damage.h>

#include <db.h>
#include <fight.h>
#include <magic.h>
#include <mob_cmds.h>

#include <math.h>
#include <stdlib.h>
#include <time.h>

static Value clock_native(int arg_count, Value* args)
{
    return DOUBLE_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value marshal_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        runtime_error("marshal() takes 1 argument; %d given.", arg_count);
        return NIL_VAL;
    }

    if (!IS_RAW_PTR(args[0])) {
        // Be forgiving; marshalling a non-raw ptr is a NOOP.
        return args[0];
    }

    return marshal_raw_ptr(AS_RAW_PTR(args[0]));
}

static Value floor_native(int arg_count, Value* args)
{
    if (arg_count == 1 && IS_INT(args[0]))
        return args[0];

    if (arg_count != 1 || !IS_DOUBLE(args[0])) {
        runtime_error("floor() takes a single double argument.");
        return NIL_VAL;
    }

    return INT_VAL((int32_t)floor(AS_DOUBLE(args[0])));
}

static Value string_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        runtime_error("string() takes 1 argument; %d given.", arg_count);
        return NIL_VAL;
    }

    char* str = string_value(args[0]);
    return OBJ_VAL(take_string(str, (int)strlen(str)));
}

static Value damage_native(int arg_count, Value* args)
{
    if (arg_count != 6) {
        runtime_error("damage() takes 6 arguments; %d given.", arg_count);
        return FALSE_VAL;
    }

    // Add support for arbitrary damage sources, later.
    if (args[0] != NIL_VAL && !IS_MOBILE(args[0])) {
        runtime_error("damage(): Expected acting Mobile for first argument.");
        return FALSE_VAL;
    }

    if (args[1] != NIL_VAL && !IS_MOBILE(args[1])) {
        runtime_error("damage(): Expected target Mobile for second argument.");
        return FALSE_VAL;
    }

    if (!IS_INT(args[2]) || !IS_INT(args[3]) || !IS_INT(args[4]) || !IS_BOOL(args[5])) {
        runtime_error("damage(): Expected integer values for third through sixth arguments.");
        return FALSE_VAL;
    }

    Mobile* ch = AS_MOBILE(args[0]);
    Mobile* victim = AS_MOBILE(args[1]);
    int dam = AS_INT(args[2]);
    int16_t dt = (int16_t)AS_INT(args[3]);
    DamageType dam_type = (DamageType)AS_INT(args[4]);
    bool show = AS_BOOL(args[5]);

    bool retval = damage(ch, victim, dam, dt, dam_type, show);

    return BOOL_VAL(retval);
}

// int dice(int number, int size)
static Value dice_native(int arg_count, Value* args)
{
    if (arg_count != 2) {
        runtime_error("dice() takes 2 arguments; %d given.", arg_count);
        return FALSE_VAL;
    }

    if (!IS_INT(args[0]) || !IS_INT(args[1])) {
        runtime_error("dice(): Expected integer for both arguments.");
        return FALSE_VAL;
    }

    int number = AS_INT(args[0]);
    int size = AS_INT(args[1]);

    int retval = dice(number, size);

    return INT_VAL(retval);
}

static Value do_native(int arg_count, Value* args)
{
    if (arg_count != 1 || !IS_STRING(args[0])) {
        runtime_error("do() takes a String argument.");
        return FALSE_VAL;
    }

    if (exec_context.me == NULL) {
        runtime_error("do() can only be done by a Mobile/Player.");
        return FALSE_VAL;
    }

    const char* argument = AS_STRING(args[0])->chars;

    // Make this command safe for scripted string literals
    char* cmd = str_dup(argument);

    interpret(exec_context.me, cmd);

    free_string(cmd);

    return TRUE_VAL;
}

// bool saves_spell(LEVEL level, Mobile* victim, DamageType dam_type)
static Value saves_spell_native(int arg_count, Value* args)
{
    if (arg_count != 3) {
        runtime_error("saves_spell() takes 3 arguments; %d given.", arg_count);
        return FALSE_VAL;
    }

    if (!IS_INT(args[0]) || !IS_INT(args[2])) {
        runtime_error("saves_spell(): Expected target integer for first and third arguments.");
        return FALSE_VAL;
    }

    // Add support for arbitrary damage sources, later.
    if (args[1] == NIL_VAL || !IS_MOBILE(args[1])) {
        runtime_error("saves_spell(): Expected target Mobile for second argument.");
        return FALSE_VAL;
    }

    LEVEL level = (int16_t)AS_INT(args[0]);
    Mobile* victim = AS_MOBILE(args[1]);
    DamageType dam_type = (DamageType)AS_INT(args[2]);

    bool retval = saves_spell(level, victim, dam_type);

    return BOOL_VAL(retval);
}

static Value delay_native(int arg_count, Value* args)
{
    if (arg_count != 2 || !IS_INT(args[0]) || !IS_CLOSURE(args[1])) {
        runtime_error("delay(): Expected interval and closure as arguments.");
        return FALSE_VAL;
    }

    int32_t interval = AS_INT(args[0]);
    ObjClosure* closure = AS_CLOSURE(args[1]);

    add_event_timer(closure, interval);

    return TRUE_VAL;
}

const NativeFuncEntry native_func_entries[] = {
    { "clock",          clock_native                },
    { "damage",         damage_native               },
    { "do",             do_native                   },
    { "dice",           dice_native                 },
    { "marshal",        marshal_native              },
    { "saves_spell",    saves_spell_native          },
    { "string",         string_native               },
    { "floor",          floor_native                },
    { "delay",          delay_native                },
    { NULL,             NULL                        },
};
