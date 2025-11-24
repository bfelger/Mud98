////////////////////////////////////////////////////////////////////////////////
// persist/persist_io.h
// Generic persistence I/O abstractions (stream ops, readers, writers).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__PERSIST_IO_H
#define MUD98__PERSIST__PERSIST_IO_H

#include <stdbool.h>
#include <stddef.h>

typedef struct persist_stream_ops_t {
    int (*getc)(void* ctx);           // Returns next byte, or EOF on end/error.
    int (*ungetc)(int ch, void* ctx); // Mirrors ungetc semantics; may be NULL if unused.
    bool (*eof)(void* ctx);           // True when no more data; may be NULL.
} PersistStreamOps;

typedef struct persist_reader_t {
    const PersistStreamOps* ops;
    void* ctx;
    const char* name; // Filename or label for error reporting; may be NULL.
} PersistReader;

typedef struct persist_writer_ops_t {
    int (*putc)(int ch, void* ctx);   // Returns written char, or EOF on error.
    size_t (*write)(const void* buffer, size_t size, void* ctx); // Optional bulk write; may be NULL.
    bool (*flush)(void* ctx);         // Optional; may be NULL.
} PersistWriterOps;

typedef struct persist_writer_t {
    const PersistWriterOps* ops;
    void* ctx;
    const char* name; // Filename or label for error reporting; may be NULL.
} PersistWriter;

#endif // !MUD98__PERSIST__PERSIST_IO_H
