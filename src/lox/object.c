////////////////////////////////////////////////////////////////////////////////
// object.c
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>

#include "db.h"

#include "lox/memory.h"
#include "lox/object.h"
#include "lox/table.h"
#include "lox/value.h"
#include "lox/vm.h"

extern char* string_space;
extern char* top_string;
extern char str_empty[1];

Obj* allocate_object(size_t size, ObjType type)
{
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->is_marked = false;

    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    lox_printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

ObjBoundMethod* new_bound_method(Value receiver, ObjClosure* method)
{
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass* new_class(ObjString* name)
{
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    init_table(&klass->methods);
    return klass;
}

ObjClosure* new_closure(ObjFunction* function)
{
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*,  function->upvalue_count);
    for (int i = 0; i < function->upvalue_count; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;
    return closure;
}

ObjFunction* new_function()
{
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalue_count = 0;
    function->name = NULL;
    init_chunk(&function->chunk);
    return function;
}

ObjInstance* new_instance(ObjClass* klass)
{
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    init_table(&instance->fields);
    return instance;
}

ObjNative* new_native(NativeFn function)
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

ObjRawPtr* new_raw_ptr(uintptr_t addr, RawType type)
{
    ObjRawPtr* ptr = ALLOCATE_OBJ(ObjRawPtr, OBJ_RAW_PTR);
    ptr->addr = addr;
    ptr->type = type;
    return ptr;
}

Value marshal_raw_ptr(ObjRawPtr* ptr)
{
    switch (ptr->type) {
    case RAW_I16:
        return NUMBER_VAL((double)(*((int16_t*)ptr->addr)));
    case RAW_I32:
        return NUMBER_VAL((double)(*((int32_t*)ptr->addr)));
    case RAW_U64:
        return NUMBER_VAL((double)(*((uint64_t*)ptr->addr)));
    case RAW_STR: {
            char** str = (char**)ptr->addr;
            return OBJ_VAL(copy_string(*str, (int)strlen(*str)));
        }
    case RAW_OBJ:
        return NUMBER_VAL((double)((uintptr_t)ptr->addr));
    default:
        runtime_error("Could not box raw value of unexpected type.");
        return NIL_VAL;
    }
}

void unmarshal_raw_val(ObjRawPtr* ptr, Value val)
{
    switch (ptr->type) {
    case RAW_OBJ: ptr->addr = (uintptr_t)(AS_NUMBER(val)); break;
    case RAW_I16: *((int16_t*)ptr->addr) = (int16_t)(AS_NUMBER(val)); break;
    case RAW_I32: *((int32_t*)ptr->addr) = (int32_t)(AS_NUMBER(val)); break;
    case RAW_U64: *((uint64_t*)ptr->addr) = (uint64_t)(AS_NUMBER(val)); break;
    case RAW_STR: {
            char* new_str = AS_CSTRING(val);
            char** old_str = (char**)ptr->addr;
            free_string(*old_str);
            *old_str = str_dup(new_str);
            break;
        }
    }
}

static ObjString* allocate_string(char* chars, int length, uint32_t hash)
{
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    push(OBJ_VAL(string));
    table_set(&vm.strings, string, NIL_VAL);
    pop();

    return string;
}

static uint32_t hash_string(const char* key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* take_string(char* chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    ObjString* interned = table_find_string(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        if (chars != &str_empty[0]
            && (chars < string_space || chars >= top_string))
            FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocate_string(chars, length, hash);
}

ObjString* copy_string(const char* chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    ObjString* interned = table_find_string(&vm.strings, chars, length, hash);
    if (interned != NULL) 
        return interned;

    if (chars >= string_space && chars < top_string) {
        // Yes, I'm casting away const.
        // It's not helpful here.
        return allocate_string((char*)chars, length, hash);
    }

    char* heap_chars = ALLOCATE(char, (size_t)length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return allocate_string(heap_chars, length, hash);
}

ObjUpvalue* new_upvalue(Value* slot)
{
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

static void print_function(ObjFunction* function)
{
    if (function->name == NULL) {
        lox_printf("<script>");
        return;
    }
    lox_printf("<fn %s>", function->name->chars);
}

void print_object(Value value)
{
    static char buf[1024];

    switch (OBJ_TYPE(value)) {
    case OBJ_ARRAY: {
        ValueArray* array_ = AS_ARRAY(value);
            lox_printf("[");
            for (int i = 0; i < array_->count; ++i) {
                if (i > 0)
                    lox_printf(",");
                print_value(array_->values[i]);
            }
            lox_printf("]");
            break;
        }
    case OBJ_BOUND_METHOD:
        print_function(AS_BOUND_METHOD(value)->method->function);
        break;
    case OBJ_CLASS:
        lox_printf("%s", AS_CLASS(value)->name->chars);
        break;
    case OBJ_CLOSURE:
        print_function(AS_CLOSURE(value)->function);
        break;
    case OBJ_FUNCTION:
        print_function(AS_FUNCTION(value));
        break;
    case OBJ_INSTANCE:
        lox_printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
        break;
    case OBJ_NATIVE:
        lox_printf("<native fn>");
        break;
    case OBJ_RAW_PTR: {
            ObjRawPtr* raw_ptr = AS_RAW_PTR(value);
            switch (raw_ptr->type) {
            case RAW_OBJ: lox_printf("<raw_obj %" PRIxPTR ">", raw_ptr->addr); break;
            case RAW_I16: lox_printf("<raw_i16 %d>", *((int16_t*)raw_ptr->addr)); break;
            case RAW_I32: lox_printf("<raw_i32 %d>", *((int32_t*)raw_ptr->addr)); break;
            case RAW_U64: lox_printf("<raw_u64 %llu>", *((uint64_t*)raw_ptr->addr)); break;
            case RAW_STR: lox_printf("<raw_str %s>", *((char**)raw_ptr->addr)); break;
            }
            break;
        }
    case OBJ_STRING:
        lox_printf("%s", AS_CSTRING(value));
        break;
    case OBJ_UPVALUE:
       lox_printf("upvalue");
        break;
    //
    case OBJ_AREA:
        lox_printf("<area %s (%d)>", NAME_STR(AS_AREA(value)), 
            VNUM_FIELD(AS_AREA(value)));
        break;
    case OBJ_AREA_DATA:
        lox_printf("<area_data %s (%d)>", NAME_STR(AS_AREA_DATA(value)),
            VNUM_FIELD(AS_AREA_DATA(value)));
        break;
    case OBJ_ROOM:
        lox_printf("<room %s (%d)>", NAME_STR(AS_ROOM(value)),
            VNUM_FIELD(AS_ROOM(value)));
        break;
    case OBJ_ROOM_DATA:
        lox_printf("<room_data %s (%d)>", NAME_STR(AS_ROOM_DATA(value)),
            VNUM_FIELD(AS_ROOM_DATA(value)));
        break;
    case OBJ_OBJ:
        lox_printf("<obj %s (%d)>", NAME_STR(AS_OBJECT(value)), 
            VNUM_FIELD(AS_OBJECT(value)));
        break;
    case OBJ_OBJ_PROTO:
        lox_printf("<obj_proto %s (%d)>", NAME_STR(AS_OBJ_PROTO(value)),
            VNUM_FIELD(AS_OBJ_PROTO(value)));
        break;
    case OBJ_MOB:
        lox_printf("<mob %s (%d)>", NAME_STR(AS_MOBILE(value)),
            VNUM_FIELD(AS_MOBILE(value)));
        break;
    case OBJ_MOB_PROTO:
        lox_printf("<mob_proto %s (%d)>", NAME_STR(AS_MOB_PROTO(value)),
            VNUM_FIELD(AS_MOB_PROTO(value)));
        break;
    } // end switch
}

void init_header(EntityHeader* header, ObjType type)
{
    header->obj.type = type;
    init_table(&header->fields);
}
