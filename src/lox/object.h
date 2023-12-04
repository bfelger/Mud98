////////////////////////////////////////////////////////////////////////////////
// object.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_object_h
#define clox_object_h

#include "lox/common.h"
#include "lox/chunk.h"
#include "lox/table.h"
#include "lox/value.h"

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)

#define IS_ARRAY(value)         is_obj_type(value, OBJ_ARRAY)
#define IS_BOUND_METHOD(value)  is_obj_type(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)         is_obj_type(value, OBJ_CLASS)
#define IS_CLOSURE(value)       is_obj_type(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)      is_obj_type(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)      is_obj_type(value, OBJ_INSTANCE)
#define IS_NATIVE(value)        is_obj_type(value, OBJ_NATIVE)
#define IS_RAW_PTR(value)       is_obj_type(value, OBJ_RAW_PTR)
#define IS_STRING(value)        is_obj_type(value, OBJ_STRING)

#define AS_ARRAY(value)         ((ObjArray*)AS_OBJ(value))
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)         ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
#define AS_RAW_PTR(value)       ((ObjRawPtr*)AS_OBJ(value))
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_ARRAY,
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_RAW_PTR,
    OBJ_STRING,
    OBJ_UPVALUE,
} ObjType;

struct Obj {
    ObjType type;
    bool is_marked;
    struct Obj* next;
};

typedef struct {
    Obj obj;
    ValueArray val_array;
} ObjArray;

typedef struct {
    Obj obj;
    int arity;
    int upvalue_count;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

typedef Value(*NativeFn)(int arg_count, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

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

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalue_count;
} ObjClosure;

typedef struct {
    Obj obj;
    ObjString* name;
    Table methods;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass* klass;
    Table fields;
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure* method;
} ObjBoundMethod;

ObjArray* new_obj_array();
ObjBoundMethod* new_bound_method(Value receiver, ObjClosure* method);
ObjClass* new_class(ObjString* name);
ObjClosure* new_closure(ObjFunction* function);
ObjFunction* new_function();
ObjInstance* new_instance(ObjClass* klass);
ObjNative* new_native(NativeFn function);
ObjRawPtr* new_raw_ptr(uintptr_t addr, RawType type);
Value marshal_raw_ptr(ObjRawPtr* ptr);
void unmarshal_raw_val(ObjRawPtr* ptr, Value val);
ObjString* take_string(char* chars, int length);
ObjString* copy_string(const char* chars, int length);
ObjUpvalue* new_upvalue(Value* slot);
void print_object(Value value);

static inline bool is_obj_type(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
