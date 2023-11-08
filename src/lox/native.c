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

static Value string_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        printf("string() takes 1 argument; %d given.", arg_count);
        return NIL_VAL;
    }

    char* str = string_value(args[0]);
    return OBJ_VAL(copy_string(str, (int)strlen(str)));
}

const NativeFuncEntry native_funcs[] = {
    { "clock",          clock_native        },
    { "marshal",        marshal_native      },
    { "string",         string_native       },
    { "get_room",       get_room_native     },
    { NULL,             NULL                },
};

ObjClass* find_class(const char* class_name)
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

void init_native_classes()
{
    init_mobile_class();
    init_room_class();
}
