////////////////////////////////////////////////////////////////////////////////
// native.h
// Extra native functions for Lox
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <time.h>

#include "lox/native.h"
#include "lox/value.h"
#include "lox/vm.h"

static Value clock_native(int arg_count, Value* args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

const NativeFuncEntry native_funcs[] = {
    { "clock",  clock_native    },
    { NULL,     NULL            },
};

static void init_mobile_class()
{
    char* source = 
        "class Mobile { "
        "   init(addr, name, vnum) { "
        "       this.addr = addr; "
        "       this.name = name; "
        "       this.vnum = vnum; "
        "   }"
        "};";

    InterpretResult result = interpret_code(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

static void init_room_class()
{
    char* source = 
        "class Room { "
        "   init(addr, name, vnum) { "
        "       this.addr = addr; "
        "       this.name = name; "
        "       this.vnum = vnum; "
        "   }"
        "};";

    InterpretResult result = interpret_code(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

void init_native_classes()
{
    init_mobile_class();
    init_room_class();
}
