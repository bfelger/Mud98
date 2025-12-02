////////////////////////////////////////////////////////////////////////////////
// persist/json/persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__JSON__PERSIST_JSON_H
#define MUD98__PERSIST__JSON__PERSIST_JSON_H

#include <merc.h>

#include <persist/persist_io.h>
#include <persist/persist_result.h>

#include <tables.h>

#include <jansson/jansson.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct reader_buffer_t {
    char* data;
    size_t len;
    size_t cap;
} ReaderBuffer;

PersistResult json_not_supported(const char* msg);
bool reader_fill_buffer(const PersistReader* reader, ReaderBuffer* out); 
bool writer_write_all(const PersistWriter* writer, const char* data, size_t len);

const char* size_name(MobSize size);

json_t* flags_to_array(FLAGS flags, const struct flag_type* table);
FLAGS flags_from_array(json_t* arr, const struct flag_type* table);
int64_t json_int_or_default(json_t* obj, const char* key, int64_t def);

#endif // MUD98__PERSIST__JSON__PERSIST_JSON_H
