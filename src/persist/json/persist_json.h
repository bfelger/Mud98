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

#include <jansson.h>

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
json_t* flags_to_array_with_defaults(FLAGS flags, const struct flag_type* defaults, const struct flag_type* table);
FLAGS flags_from_array_with_defaults(json_t* arr, const struct flag_type* defaults, const struct flag_type* table);
int64_t json_int_or_default(json_t* obj, const char* key, int64_t def);

// JSON HELPER MACROS //////////////////////////////////////////////////////////

#define JSON_STRING(json_obj, key) \
    json_string_value(json_object_get((json_obj), (key)))

#define JSON_INTERN(str, tgt) \
    if (str) { free_string(tgt); tgt = boot_intern_string(str); }

#define JSON_FLAGS(json_obj, key, table) \
    flags_from_array(json_object_get((json_obj), (key)), (table))

#define JSON_SET_INT(json_obj, key, value) \
    json_object_set_new((json_obj), (key), json_integer(value))

#define JSON_SET_STRING(json_obj, key, value) \
    json_object_set_new((json_obj), (key), json_string(value))

#endif // MUD98__PERSIST__JSON__PERSIST_JSON_H
