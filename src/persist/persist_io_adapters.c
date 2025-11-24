////////////////////////////////////////////////////////////////////////////////
// persist/persist_io_adapters.c
// Implementations of common PersistReader/PersistWriter adapters.
////////////////////////////////////////////////////////////////////////////////

#include "persist_io_adapters.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FILE* stream ops.
static int file_getc(void* ctx) { return getc((FILE*)ctx); }
static int file_ungetc(int ch, void* ctx) { return ungetc(ch, (FILE*)ctx); }
static bool file_eof(void* ctx) { return feof((FILE*)ctx) != 0; }

const PersistStreamOps PERSIST_FILE_STREAM_OPS = {
    .getc = file_getc,
    .ungetc = file_ungetc,
    .eof = file_eof,
};

static int file_putc(int ch, void* ctx) { return putc(ch, (FILE*)ctx); }
static size_t file_write(const void* buffer, size_t size, void* ctx) { return fwrite(buffer, 1, size, (FILE*)ctx); }
static bool file_flush(void* ctx) { return fflush((FILE*)ctx) == 0; }

const PersistWriterOps PERSIST_FILE_WRITER_OPS = {
    .putc = file_putc,
    .write = file_write,
    .flush = file_flush,
};

// Buffer reader ops.
static int buffer_getc(void* ctx)
{
    PersistBufferReaderCtx* state = (PersistBufferReaderCtx*)ctx;
    if (state->pos >= state->len)
        return EOF;
    return state->data[state->pos++];
}

static int buffer_ungetc(int ch, void* ctx)
{
    PersistBufferReaderCtx* state = (PersistBufferReaderCtx*)ctx;
    if (state->pos == 0)
        return EOF;
    state->pos--;
    // Ignore `ch`; we restore previous byte implicitly.
    return ch;
}

static bool buffer_eof(void* ctx)
{
    PersistBufferReaderCtx* state = (PersistBufferReaderCtx*)ctx;
    return state->pos >= state->len;
}

const PersistStreamOps PERSIST_BUFFER_STREAM_OPS = {
    .getc = buffer_getc,
    .ungetc = buffer_ungetc,
    .eof = buffer_eof,
};

// Buffer writer ops.
static bool ensure_buffer_capacity(PersistBufferWriter* buf, size_t extra)
{
    size_t needed = buf->len + extra;
    if (needed <= buf->cap)
        return true;

    size_t new_cap = buf->cap == 0 ? 128 : buf->cap;
    while (new_cap < needed)
        new_cap *= 2;

    unsigned char* new_data = realloc(buf->data, new_cap);
    if (!new_data)
        return false;

    buf->data = new_data;
    buf->cap = new_cap;
    return true;
}

static int buffer_putc(int ch, void* ctx)
{
    PersistBufferWriter* buf = (PersistBufferWriter*)ctx;
    if (!ensure_buffer_capacity(buf, 1))
        return EOF;
    buf->data[buf->len++] = (unsigned char)ch;
    return ch;
}

static size_t buffer_write(const void* buffer, size_t size, void* ctx)
{
    PersistBufferWriter* buf = (PersistBufferWriter*)ctx;
    if (!ensure_buffer_capacity(buf, size))
        return 0;
    memcpy(buf->data + buf->len, buffer, size);
    buf->len += size;
    return size;
}

static bool buffer_flush(void* ctx)
{
    (void)ctx;
    return true;
}

const PersistWriterOps PERSIST_BUFFER_WRITER_OPS = {
    .putc = buffer_putc,
    .write = buffer_write,
    .flush = buffer_flush,
};
