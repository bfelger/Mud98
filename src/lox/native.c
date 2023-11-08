////////////////////////////////////////////////////////////////////////////////
// native.h
// Extra native functions for Lox
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <time.h>

#include "entities/room.h"

#include "lox/native.h"
#include "lox/value.h"
#include "lox/vm.h"

static Value clock_native(int arg_count, Value* args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value marshal_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        printf("marshal() takes 1 argument; %d given.", arg_count);
        return NIL_VAL;
    }

    if (!IS_RAW_PTR(args[0])) {
        // Be forgiving; marshalling a non-raw ptr is a NOOP.
        return args[0];
    }

    return marshal_raw_ptr(AS_RAW_PTR(args[0]));
}

const NativeFuncEntry native_funcs[] = {
    { "clock",      clock_native    },
    { "marshal",    marshal_native  },
    { NULL,         NULL            },
};

static ObjClass* find_class(const char* class_name)
{
    ObjString* name = copy_string(class_name, (int)strlen(class_name));
    Value value;
    if (!table_get(&vm.globals, name, &value)) {
        printf("Undefined variable '%s'.", name->chars);
        exit(70);
    }

    if (!IS_CLASS(value)) {
        printf("'%s' is not a class.", name->chars);
        exit(70);
    }

    return AS_CLASS(value);
}

static void init_mobile_class()
{
    char* source = 
        "class Mobile { "
        "}";

    InterpretResult result = interpret_code(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

static ObjClass* room_class = NULL;

static void init_room_class()
{
    char* source = 
        "class Room { "
        "   name() { return marshal(this._name); }"
        "   vnum() { return marshal(this._vnum); }"
        "}";

    InterpretResult result = interpret_code(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);

    room_class = find_class("Room");
}

#define SET_NATIVE_FIELD(inst, src, tgt, TYPE)                                 \
{                                                                              \
    Value tgt##_value = WRAP_##TYPE(src);                                      \
    push(tgt##_value);                                                         \
    char* tgt##_str = "_" #tgt;                                                \
    ObjString* tgt##_fld = copy_string(tgt##_str, (int)strlen(tgt##_str)); \
    push(OBJ_VAL(tgt##_fld));                                                  \
    table_set(&(inst)->fields, tgt##_fld, tgt##_value);                        \
    pop();                                                                     \
    pop();                                                                     \
}

Value create_room_value(Room* room)
{
    if (!room || !room_class)
        return NIL_VAL;

    ObjInstance* inst = new_instance(room_class);
    push(OBJ_VAL(inst));

    SET_NATIVE_FIELD(inst, room->data->name, name, STR);
    SET_NATIVE_FIELD(inst, room->data->vnum, vnum, I32);

    pop(); // instance

    return OBJ_VAL(inst);
}

void init_native_classes()
{
    init_mobile_class();
    init_room_class();
}
