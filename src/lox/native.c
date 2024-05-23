////////////////////////////////////////////////////////////////////////////////
// native.h
// Extra native functions for Lox
////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "entities/area.h"
#include "entities/mobile.h"
#include "entities/object.h"
#include "entities/room.h"

#include "lox/compiler.h"
#include "lox/native.h"
#include "lox/value.h"
#include "lox/vm.h"

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

static Value is_obj_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        runtime_error("is_obj() takes 1 argument; %d given.", arg_count);
        return FALSE_VAL;
    }

    if (IS_OBJECT(args[0]))
        return TRUE_VAL;
    
    return FALSE_VAL;
}

static Value is_obj_proto_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        runtime_error("is_obj_proto() takes 1 argument; %d given.", arg_count);
        return FALSE_VAL;
    }

    if (IS_OBJ_PROTO(args[0]))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_mob_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        runtime_error("is_mob() takes 1 argument; %d given.", arg_count);
        return FALSE_VAL;
    }

    if (IS_MOBILE(args[0]))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_mob_proto_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        runtime_error("is_mob_proto() takes 1 argument; %d given.", arg_count);
        return FALSE_VAL;
    }

    if (IS_MOB_PROTO(args[0]))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_room_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        runtime_error("is_room() takes 1 argument; %d given.", arg_count);
        return FALSE_VAL;
    }

    if (IS_ROOM(args[0]))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_room_data_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        runtime_error("is_room_data() takes 1 argument; %d given.", arg_count);
        return FALSE_VAL;
    }

    if (IS_ROOM_DATA(args[0]))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_area_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        runtime_error("is_area() takes 1 argument; %d given.", arg_count);
        return FALSE_VAL;
    }

    if (IS_AREA(args[0]))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_area_data_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        runtime_error("is_area_data() takes 1 argument; %d given.", arg_count);
        return FALSE_VAL;
    }

    if (IS_AREA_DATA(args[0]))
        return TRUE_VAL;

    return FALSE_VAL;
}

const NativeFuncEntry native_funcs[] = {
    { "clock",          clock_native                },
    { "marshal",        marshal_native              },
    { "string",         string_native               },
    { "is_area",        is_area_native              },
    { "is_area_data",   is_area_data_native         },
    { "is_mob",         is_mob_native               },
    { "is_mob_proto",   is_mob_proto_native         },
    { "is_obj",         is_obj_native               },
    { "is_obj_proto",   is_obj_proto_native         },
    { "is_room",        is_room_native              },
    { "is_room_data",   is_room_data_native         },
    { "floor",          floor_native                },
    { NULL,             NULL                        },
};

ObjClass* find_class(const char* class_name)
{
    ObjString* name = copy_string(class_name, (int)strlen(class_name));
    Value value;
    if (!table_get(&vm.globals, name, &value)) {
        runtime_error("Undefined variable '%s'.", name->chars);
        exit(70);
    }

    if (!IS_CLASS(value)) {
        runtime_error("'%s' is not a class.", name->chars);
        exit(70);
    }

    return AS_CLASS(value);
}

static void define_native(const char* name, NativeFn function)
{
    push(OBJ_VAL(copy_string(name, (int)strlen(name))));
    push(OBJ_VAL(new_native(function)));
    table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void add_global(const char* name, Value val)
{
    push(OBJ_VAL(copy_string(name, (int)strlen(name))));
    push(val);
    table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void init_natives()
{
    for (int i = 0; native_funcs[i].name != NULL; ++i)
        define_native(native_funcs[i].name, native_funcs[i].func);

    init_damage_consts();

    add_global("global_areas", OBJ_VAL(&global_areas));
    add_global("global_rooms", OBJ_VAL(&global_rooms));
    add_global("mob_protos", OBJ_VAL(&mob_protos));
    add_global("obj_protos", OBJ_VAL(&obj_protos));
    add_global("global_objs", OBJ_VAL(&obj_list));
    add_global("global_mobs", OBJ_VAL(&mob_list));
}
