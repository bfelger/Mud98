////////////////////////////////////////////////////////////////////////////////
// object.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_object_h
#define clox_object_h

#include "lox/common.h"
#include "lox/value.h"

typedef struct area_t Area;
typedef struct area_data_t AreaData;
typedef struct room_t Room;
typedef struct room_data_t RoomData;
typedef struct object_t Object;
typedef struct obj_prototype_t ObjPrototype;
typedef struct mobile_t Mobile;
typedef struct mob_prototype_t MobPrototype;

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)

#define IS_ARRAY(value)         is_obj_type(value, OBJ_ARRAY)
#define IS_BOUND_METHOD(value)  is_obj_type(value, OBJ_BOUND_METHOD)
#define IS_NATIVE_METHOD(value) is_obj_type(value, OBJ_NATIVE_METHOD)
#define IS_NATIVE_CMD(value)    is_obj_type(value, OBJ_NATIVE_CMD)
#define IS_CLASS(value)         is_obj_type(value, OBJ_CLASS)
#define IS_CLOSURE(value)       is_obj_type(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)      is_obj_type(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)      is_obj_type(value, OBJ_INSTANCE)
#define IS_NATIVE(value)        is_obj_type(value, OBJ_NATIVE)
#define IS_RAW_PTR(value)       is_obj_type(value, OBJ_RAW_PTR)
#define IS_STRING(value)        is_obj_type(value, OBJ_STRING)
#define IS_TABLE(value)         is_obj_type(value, OBJ_TABLE)
#define IS_LIST(value)          is_obj_type(value, OBJ_LIST)
//
#define IS_AREA(value)          is_obj_type(value, OBJ_AREA)
#define IS_AREA_DATA(value)     is_obj_type(value, OBJ_AREA_DATA)
#define IS_ROOM(value)          is_obj_type(value, OBJ_ROOM)
#define IS_ROOM_DATA(value)     is_obj_type(value, OBJ_ROOM_DATA)
#define IS_OBJECT(value)        is_obj_type(value, OBJ_OBJ)
#define IS_OBJ_PROTO(value)     is_obj_type(value, OBJ_OBJ_PROTO)
#define IS_MOBILE(value)        is_obj_type(value, OBJ_MOB)
#define IS_MOB_PROTO(value)     is_obj_type(value, OBJ_MOB_PROTO)
#define IS_ENTITY(value)        (IS_OBJ(value) && AS_OBJ(value)->type >= 100)   

#define AS_ARRAY(value)         ((ValueArray*)AS_OBJ(value))
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)AS_OBJ(value))
#define AS_NATIVE_METHOD(value) ((ObjNativeMethod*)AS_OBJ(value))
#define AS_NATIVE_CMD(value)    ((ObjNativeCmd*)AS_OBJ(value))
#define AS_CLASS(value)         ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
#define AS_RAW_PTR(value)       ((ObjRawPtr*)AS_OBJ(value))
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)
#define AS_TABLE(value)         ((Table*)AS_OBJ(value))
#define AS_LIST(value)          ((List*)AS_OBJ(value))
//
#define AS_ENTITY(value)        ((EntityHeader*)AS_OBJ(value))
#define AS_AREA(value)          ((Area*)AS_OBJ(value))
#define AS_AREA_DATA(value)     ((AreaData*)AS_OBJ(value))
#define AS_ROOM(value)          ((Room*)AS_OBJ(value))
#define AS_ROOM_DATA(value)     ((RoomData*)AS_OBJ(value))
#define AS_OBJECT(value)        ((Object*)AS_OBJ(value))
#define AS_OBJ_PROTO(value)     ((ObjPrototype*)AS_OBJ(value))
#define AS_MOBILE(value)        ((Mobile*)AS_OBJ(value))
#define AS_MOB_PROTO(value)     ((MobPrototype*)AS_OBJ(value))

typedef enum {
    OBJ_ARRAY,
    OBJ_BOUND_METHOD,
    OBJ_NATIVE_METHOD,
    OBJ_NATIVE_CMD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_RAW_PTR,
    OBJ_STRING,
    OBJ_UPVALUE,
    OBJ_TABLE,
    OBJ_LIST,
    //
    OBJ_AREA        = 100,
    OBJ_AREA_DATA   = 101,
    OBJ_ROOM        = 102,
    OBJ_ROOM_DATA   = 103,
    OBJ_OBJ         = 104,
    OBJ_OBJ_PROTO   = 105,
    OBJ_MOB         = 106,
    OBJ_MOB_PROTO   = 107,
} ObjType;

struct Obj {
    ObjType type;
    bool is_marked;
    struct Obj* next;
};

typedef enum {
    RAW_OBJ,
    RAW_I16,
    RAW_I32,
    RAW_U64,
    RAW_STR,
} RawType;

#define WRAP_OBJ(val)        OBJ_VAL(new_raw_ptr((uintptr_t)(val), RAW_OBJ));
#define WRAP_I16(val)        OBJ_VAL(new_raw_ptr((uintptr_t)&(val), RAW_I16));
#define WRAP_I32(val)        OBJ_VAL(new_raw_ptr((uintptr_t)&(val), RAW_I32));
#define WRAP_U64(val)        OBJ_VAL(new_raw_ptr((uintptr_t)&(val), RAW_U64));
#define WRAP_STR(val)        OBJ_VAL(new_raw_ptr((uintptr_t)&(val), RAW_STR));

typedef struct {
    Obj obj;
    uintptr_t addr;
    RawType type;
} ObjRawPtr;

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

Obj* allocate_object(size_t size, ObjType type);
ObjRawPtr* new_raw_ptr(uintptr_t addr, RawType type);
Value marshal_raw_ptr(ObjRawPtr* ptr);
void unmarshal_raw_val(ObjRawPtr* ptr, Value val);
ObjString* take_string(char* chars, int length);
ObjString* copy_string(const char* chars, int length);
ObjUpvalue* new_upvalue(Value* slot);
void print_object(Value value);

#define ALLOCATE_OBJ(type, object_type) \
    (type*)allocate_object(sizeof(type), object_type)

static inline bool is_obj_type(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
