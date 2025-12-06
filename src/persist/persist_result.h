////////////////////////////////////////////////////////////////////////////////
// persist/persist_result.h
// Generic persistence result/status helpers.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__PERSIST_RESULT_H
#define MUD98__PERSIST__PERSIST_RESULT_H

#include <stdbool.h>

typedef enum persist_status_t {
    PERSIST_OK = 0,
    PERSIST_ERR_IO,
    PERSIST_ERR_FORMAT,
    PERSIST_ERR_UNSUPPORTED,
    PERSIST_ERR_INTERNAL,
} PersistStatus;

typedef struct persist_result_t {
    PersistStatus status;
    const char* message; // Caller owns storage; can be static.
    int line;            // Optional line number for format errors, -1 if unknown.
} PersistResult;

static inline bool persist_succeeded(PersistResult result)
{
    return result.status == PERSIST_OK;
}

#endif // !MUD98__PERSIST__PERSIST_RESULT_H
