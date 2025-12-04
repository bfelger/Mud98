////////////////////////////////////////////////////////////////////////////////
// persist/json/persist_json.c
////////////////////////////////////////////////////////////////////////////////

#include "persist_json.h"

#include <persist/persist_io.h>
#include <persist/persist_result.h>

#include <db.h>
#include <lookup.h>
#include <merc.h>
#include <tables.h>

PersistResult json_not_supported(const char* msg)
{
    PersistResult res = { PERSIST_ERR_UNSUPPORTED, msg, -1 };
    return res;
}

bool reader_fill_buffer(const PersistReader* reader, ReaderBuffer* out)
{
    ReaderBuffer buf = { 0 };
    buf.cap = 1024;
    buf.data = malloc(buf.cap);
    if (!buf.data)
        return false;

    for (;;) {
        int ch = reader->ops->getc(reader->ctx);
        if (ch == EOF)
            break;
        if (buf.len + 1 > buf.cap) {
            size_t new_cap = buf.cap * 2;
            char* tmp = realloc(buf.data, new_cap);
            if (!tmp) {
                free(buf.data);
                return false;
            }
            buf.data = tmp;
            buf.cap = new_cap;
        }
        buf.data[buf.len++] = (char)ch;
    }

    out->data = buf.data;
    out->len = buf.len;
    out->cap = buf.cap;
    return true;
}

bool writer_write_all(const PersistWriter* writer, const char* data, size_t len)
{
    if (writer->ops->write) {
        return writer->ops->write(data, len, writer->ctx) == len;
    }
    // Fallback to putc loop.
    for (size_t i = 0; i < len; i++) {
        if (writer->ops->putc(data[i], writer->ctx) == EOF)
            return false;
    }
    return true;
}

const char* size_name(MobSize size)
{
    if (size < 0 || size >= MOB_SIZE_COUNT)
        return mob_size_table[SIZE_MEDIUM].name;
    return mob_size_table[size].name;
}

json_t* flags_to_array(FLAGS flags, const struct flag_type* table)
{
    json_t* arr = json_array();
    if (!table)
        return arr;
    for (int i = 0; table[i].name != NULL; i++) {
        if (IS_SET(flags, table[i].bit)) {
            json_array_append_new(arr, json_string(table[i].name));
        }
    }
    return arr;
}

FLAGS flags_from_array(json_t* arr, const struct flag_type* table)
{
    FLAGS flags = 0;
    if (!json_is_array(arr) || !table)
        return flags;
    size_t size = json_array_size(arr);
    for (size_t i = 0; i < size; i++) {
        const char* name = json_string_value(json_array_get(arr, i));
        if (!name)
            continue;
        FLAGS bit = NO_FLAG;
        for (int t = 0; table[t].name != NULL; ++t) {
            if (!str_cmp(name, table[t].name)) {
                bit = table[t].bit;
                break;
            }
        }
        if (bit != NO_FLAG)
            SET_BIT(flags, bit);
    }
    return flags;
}

json_t* flags_to_array_with_defaults(FLAGS flags, const struct flag_type* defaults, const struct flag_type* table)
{
    if (!defaults)
        return flags_to_array(flags, table);

    json_t* arr = json_array();
    FLAGS remaining = flags;
    for (int i = 0; defaults && defaults[i].name != NULL; i++) {
        FLAGS mask = defaults[i].bit;
        if (mask != 0 && (remaining & mask) == mask) {
            json_array_append_new(arr, json_string(defaults[i].name));
            remaining &= ~mask;
        }
    }
    if (table) {
        for (int i = 0; table[i].name != NULL; i++) {
            if (IS_SET(remaining, table[i].bit)) {
                json_array_append_new(arr, json_string(table[i].name));
                remaining &= ~table[i].bit;
            }
        }
    }
    return arr;
}

FLAGS flags_from_array_with_defaults(json_t* arr, const struct flag_type* defaults, const struct flag_type* table)
{
    if (!defaults)
        return flags_from_array(arr, table);

    FLAGS flags = 0;
    if (!json_is_array(arr))
        return flags;

    size_t size = json_array_size(arr);
    for (size_t i = 0; i < size; i++) {
        const char* name = json_string_value(json_array_get(arr, i));
        if (!name)
            continue;
        FLAGS mask = flag_lookup(name, defaults);
        if (mask != NO_FLAG) {
            flags |= mask;
            continue;
        }
        mask = flag_lookup(name, table);
        if (mask != NO_FLAG)
            flags |= mask;
    }
    return flags;
}

int64_t json_int_or_default(json_t* obj, const char* key, int64_t def)
{
    json_t* val = json_object_get(obj, key);
    if (json_is_integer(val))
        return json_integer_value(val);
    return def;
}
