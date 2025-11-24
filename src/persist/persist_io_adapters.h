////////////////////////////////////////////////////////////////////////////////
// persist/persist_io_adapters.h
// Common adapters for PersistReader/PersistWriter (FILE* and in-memory buffers).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__PERSIST_IO_ADAPTERS_H
#define MUD98__PERSIST__PERSIST_IO_ADAPTERS_H

#include "persist_io.h"

#include <stddef.h>
#include <stdio.h>

// FILE* backed reader/writer.
extern const PersistStreamOps PERSIST_FILE_STREAM_OPS;
extern const PersistWriterOps PERSIST_FILE_WRITER_OPS;

static inline PersistReader persist_reader_from_FILE(FILE* fp, const char* name)
{
    PersistReader reader = { &PERSIST_FILE_STREAM_OPS, fp, name };
    return reader;
}

static inline PersistWriter persist_writer_from_FILE(FILE* fp, const char* name)
{
    PersistWriter writer = { &PERSIST_FILE_WRITER_OPS, fp, name };
    return writer;
}

// In-memory read-only buffer reader.
typedef struct persist_buffer_reader_ctx_t {
    const unsigned char* data;
    size_t len;
    size_t pos;
} PersistBufferReaderCtx;

extern const PersistStreamOps PERSIST_BUFFER_STREAM_OPS;

static inline PersistReader persist_reader_from_buffer(const void* data, size_t len, const char* name, PersistBufferReaderCtx* ctx_out)
{
    PersistBufferReaderCtx ctx = { (const unsigned char*)data, len, 0 };
    *ctx_out = ctx;
    PersistReader reader = { &PERSIST_BUFFER_STREAM_OPS, ctx_out, name };
    return reader;
}

// In-memory write buffer writer.
typedef struct persist_buffer_writer_t {
    unsigned char* data;
    size_t len;
    size_t cap;
} PersistBufferWriter;

extern const PersistWriterOps PERSIST_BUFFER_WRITER_OPS;

// Caller owns buffer lifetime; passes in an initial buffer/capacity (may be NULL/0).
static inline PersistWriter persist_writer_from_buffer(PersistBufferWriter* buf, const char* name)
{
    PersistWriter writer = { &PERSIST_BUFFER_WRITER_OPS, buf, name };
    return writer;
}

#endif // !MUD98__PERSIST__PERSIST_IO_ADAPTERS_H
