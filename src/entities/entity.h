////////////////////////////////////////////////////////////////////////////////
// entity.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ENTITIES__ENTITY_H
#define MUD98__ENTITIES__ENTITY_H

#include <lox/lox.h>

typedef struct entity_t {
    Obj obj;
    ObjString* name;
    Table fields;
    ObjClass* klass;
    ObjString* script;
    int32_t vnum;
    List events;
    FLAGS event_triggers;
} Entity;

void init_header(Entity* header, ObjType type);
ObjClass* create_entity_class(const char* name, const char* bare_class_source);
Value is_obj_lox(Value receiver, int arg_count, Value* args);
Value is_obj_proto_lox(Value receiver, int arg_count, Value* args);
Value is_mob_lox(Value receiver, int arg_count, Value* args);
Value is_mob_proto_lox(Value receiver, int arg_count, Value* args);
Value is_room_lox(Value receiver, int arg_count, Value* args);
Value is_room_data_lox(Value receiver, int arg_count, Value* args);
Value is_area_lox(Value receiver, int arg_count, Value* args);
Value is_area_data_lox(Value receiver, int arg_count, Value* args);

#define SET_NATIVE_FIELD(inst, src, tgt, TYPE)                                 \
{                                                                              \
    Value tgt##_value = WRAP_##TYPE(src);                                      \
    push(tgt##_value);                                                         \
    char* tgt##_str = #tgt;                                                    \
    ObjString* tgt##_fld = copy_string(tgt##_str, (int)strlen(tgt##_str));     \
    push(OBJ_VAL(tgt##_fld));                                                  \
    table_set(&(inst)->fields, tgt##_fld, tgt##_value);                        \
    pop();                                                                     \
    pop();                                                                     \
}

#define SET_LOX_FIELD(inst, value, field)                                      \
{                                                                              \
    Value tgt##_value = OBJ_VAL(value);                                        \
    push(tgt##_value);                                                         \
    char* field##_str = #field;                                                \
    ObjString* key = copy_string(field##_str, (int)strlen(field##_str));       \
    push(OBJ_VAL(key));                                                        \
    table_set(&(inst)->fields, key, tgt##_value);                              \
    pop();                                                                     \
    pop();                                                                     \
}

static inline void set_name(Entity* header, ObjString* new_name)
{
    header->name = new_name;
    SET_LOX_FIELD(header, header->name, name);
}

#define SET_NAME(obj, name)     set_name(&((obj)->header), name)

#define C_STR(string)       (string->chars)

#define NAME_STR(obj)       (obj->header.name->chars)
#define NAME_FIELD(obj)     (obj->header.name)

#define VNUM_FIELD(obj)     (obj->header.vnum)

void add_event(Entity* entity, Event* event);

#endif // !MUD98__ENTITIES__ENTITY_H
