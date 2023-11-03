////////////////////////////////////////////////////////////////////////////////
// native.h
// Extra native functions for Lox
////////////////////////////////////////////////////////////////////////////////

#include <time.h>

#include "lox/native.h"
#include "lox/value.h"

static Value clock_native(int arg_count, Value* args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

const NativeFuncEntry native_funcs[] = {
    { "clock",  clock_native    },
    { NULL,     NULL            },
};

