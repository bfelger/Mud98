////////////////////////////////////////////////////////////////////////////////
// loader_guard.h
// Helpers to convert legacy loader exits into PersistResult failures.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__ROM_OLC__LOADER_GUARD_H
#define MUD98__PERSIST__ROM_OLC__LOADER_GUARD_H

#include <setjmp.h>

typedef struct loader_guard_t {
    jmp_buf env;
    const char* message;
    int line;
    bool active;
} LoaderGuard;

extern LoaderGuard* current_loader_guard;

#define WITH_LOADER_GUARD(guard_var, body)            \
    do {                                              \
        LoaderGuard guard_var = { 0 };                \
        current_loader_guard = &guard_var;            \
        if (setjmp(guard_var.env) == 0) {             \
            body;                                     \
        }                                             \
        current_loader_guard = NULL;                  \
    } while (0)

#endif // !MUD98__PERSIST__ROM_OLC__LOADER_GUARD_H
