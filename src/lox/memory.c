////////////////////////////////////////////////////////////////////////////////
// memory.c
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>

#include "merc.h"

#include "lox/compiler.h"
#include "lox/memory.h"
#include "lox/vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "lox/debug.h"
#endif

#include "entities/area.h"
#include "entities/obj_prototype.h"
#include "entities/room.h"

#define GC_HEAP_GROW_FACTOR 2

extern char* string_space;
extern char* top_string;
extern char str_empty[1];

void* reallocate(void* pointer, size_t old_size, size_t new_size)
{
    vm.bytes_allocated += new_size - old_size;
    if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
        collect_garbage();
#else
        if (vm.bytes_allocated > vm.next_gc) {
            collect_garbage();
        }
#endif
    }

    if (new_size == 0 && old_size > 0) {
        free_mem(pointer, old_size);
        return NULL;
    }

    char* result = (char*)alloc_mem(new_size);
    if (result == NULL) {
        bug("lox/memory/reallocate(): alloc_mem() failed.");
        exit(1);
    }
    if (pointer)
        memcpy(result, pointer, old_size);
    memset(result + old_size, 0, new_size - old_size);
    if (pointer)
        free_mem(pointer, old_size);
    return result;
}

void mark_object(Obj* object)
{
    if (object == NULL)
        return;
    if (object->is_marked)
        return;
#ifdef DEBUG_LOG_GC
    lox_printf("%p mark ", (void*)object);
    print_value(OBJ_VAL(object));
    lox_printf("\n");
#endif
    object->is_marked = true;

    if (vm.gray_capacity < vm.gray_count + 1) {
        vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
        // Doesn't use db.c mem routines so we don't throw off the GC
        Obj** temp = (Obj**)realloc(vm.gray_stack, sizeof(Obj*) * vm.gray_capacity);
        if (temp == NULL) {
            bug("lox/memory/mark_object: Could not allocate %zu bytes.",
                sizeof(Obj*) * vm.gray_capacity);
            exit(1);
        }
        vm.gray_stack = temp;
    }

    vm.gray_stack[vm.gray_count++] = object;
}

void mark_value(Value value)
{
    if (IS_OBJ(value))
        mark_object(AS_OBJ(value));
}

static void mark_array(ValueArray* array)
{
    for (int i = 0; i < array->count; i++) {
        mark_value(array->values[i]);
    }
}

static void mark_entity(EntityHeader* entity)
{
    if (entity == NULL)
        return;
    mark_object((Obj*)entity->name);
    mark_table(&entity->fields);
}

static void blacken_object(Obj* object)
{
#ifdef DEBUG_LOG_GC
    lox_printf("%p blacken ", (void*)object);
    print_value(OBJ_VAL(object));
    lox_printf("\n");
#endif

    switch (object->type) {
    case OBJ_ARRAY: {
        ValueArray* array_ = (ValueArray*)object;
        mark_array(array_);
        break;
    }
    case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = (ObjBoundMethod*)object;
        mark_value(bound->receiver);
        mark_object((Obj*)bound->method);
        break;
    }
    case OBJ_CLASS: {
        ObjClass* klass = (ObjClass*)object;
        mark_object((Obj*)klass->name);
        mark_table(&klass->methods);
        break;
    }
    case OBJ_CLOSURE: {
        ObjClosure* closure = (ObjClosure*)object;
        mark_object((Obj*)closure->function);
        for (int i = 0; i < closure->upvalue_count; i++) {
            mark_object((Obj*)closure->upvalues[i]);
        }
        break;
    }
    case OBJ_FUNCTION: {
        ObjFunction* function = (ObjFunction*)object;
        mark_object((Obj*)function->name);
        mark_array(&function->chunk.constants);
        break;
    }
    case OBJ_INSTANCE: {
        ObjInstance* instance = (ObjInstance*)object;
        mark_object((Obj*)instance->klass);
        mark_table(&instance->fields);
        break;
    }
    case OBJ_UPVALUE:
        mark_value(((ObjUpvalue*)object)->closed);
        break;
    case OBJ_TABLE: {
        Table* table = (Table*)object;
        mark_table(table);
        break;
    }
    case OBJ_LIST: {
        List* list = (List*)object;
        for (Node* node = list->front; node != NULL; node = node->next)
            mark_value(node->value);
        break;
    }
    case OBJ_NATIVE:
    case OBJ_RAW_PTR:
    case OBJ_STRING:
        break;
    //
    case OBJ_AREA: {
        Area* area = (Area*)object;
        mark_entity(&area->header);
        break;
    }
    case OBJ_AREA_DATA: {
        AreaData* area_data = (AreaData*)object;
        mark_entity(&area_data->header);
        break;
    }
    case OBJ_ROOM: {
        Room* room = (Room*)object;
        mark_entity(&room->header);
        break;
    }
    case OBJ_ROOM_DATA: {
        RoomData* room_data = (RoomData*)object;
        mark_entity(&room_data->header);
        break;
    }
    case OBJ_OBJ: {
        Object* obj = (Object*)object;
        mark_entity(&obj->header);
        mark_object((Obj*)obj->owner);
        break;
    }
    case OBJ_OBJ_PROTO: {
        ObjPrototype* obj_proto = (ObjPrototype*)object;
        mark_entity(&obj_proto->header);
        break;
    }
    case OBJ_MOB: {
        Mobile* mob = (Mobile*)object;
        mark_entity(&mob->header);
        break;
    }
    case OBJ_MOB_PROTO: {
        MobPrototype* mob_proto = (MobPrototype*)object;
        mark_entity(&mob_proto->header);
        break;
    }
    } // end switch
}

static void free_obj_value(Obj* object)
{
#ifdef DEBUG_LOG_GC
    lox_printf("%p free type %d\n", (void*)object, object->type);
#endif
    switch (object->type) {
    case OBJ_ARRAY:
        ValueArray* array_ = (ValueArray*)object;
        free_value_array(array_);
        FREE(ValueArray, object);
        break;
    case OBJ_BOUND_METHOD:
        FREE(ObjBoundMethod, object);
        break;
    case OBJ_CLASS: {
            ObjClass* klass = (ObjClass*)object;
            free_table(&klass->methods);
            FREE(ObjClass, object);
            break;
        }
    case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalue_count);
            FREE(ObjClosure, object);
            break;
        }
    case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            free_chunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }
    case OBJ_INSTANCE:  {
            ObjInstance* instance = (ObjInstance*)object;
            free_table(&instance->fields);
            FREE(ObjInstance, object);
            break;
        }
    case OBJ_NATIVE:
        FREE(ObjNative, object);
        break;
    case OBJ_RAW_PTR:
        FREE(ObjRawPtr, object);
        break;
    case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            char* str = string->chars;
            if (str != &str_empty[0] && (str < string_space || str >= top_string))
                FREE_ARRAY(char, string->chars, (size_t)string->length + 1);
            FREE(ObjString, object);
            break;
        }
    case OBJ_TABLE: {
        Table* table = (Table*)object;
        free_table(table);
        FREE(Table, object);
        break;
    }
    case OBJ_UPVALUE:
        FREE(ObjUpvalue, object);
        break;
    case OBJ_LIST: {
        List* list = (List*)object;
        while (list->count > 0)
            list_pop(list);
        FREE(List, object);
        break;
    }
    //
    case OBJ_AREA:
    case OBJ_AREA_DATA:
    case OBJ_ROOM:
    case OBJ_ROOM_DATA:
    case OBJ_OBJ:
    case OBJ_OBJ_PROTO:
    case OBJ_MOB:
    case OBJ_MOB_PROTO:
        break;
    } // end switch
}

static void mark_natives()
{
    AreaData* area_data;
    Area* area;
    RoomData* room_data;
    Room* room;
    ObjPrototype* obj_proto;
    Object* obj;
    MobPrototype* mob_proto;
    Mobile* mob;

    mark_array(&global_areas);

    FOR_EACH_AREA(area_data) {
        mark_entity(&area_data->header);
        mark_list(&area_data->instances);
        FOR_EACH_AREA_INST(area, area_data) {
            mark_entity(&area->header);
            mark_table(&area->rooms);
        }
    }

    FOR_EACH_GLOBAL_ROOM_DATA(room_data) {
        mark_entity(&room_data->header);
        FOR_EACH(room, room_data->instances) {
            mark_entity(&room->header);
            mark_list(&room->objects);
            mark_list(&room->mobiles);
        }
    }

    FOR_EACH_OBJ_PROTO(obj_proto) {
        mark_entity(&obj_proto->header);
    }

    FOR_EACH(obj, obj_list) {
        mark_entity(&obj->header);
        if (obj->owner != NULL)
            mark_object((Obj*)obj->owner);
        mark_list(&obj->objects);
    }

    FOR_EACH_MOB_PROTO(mob_proto) {
        mark_entity(&mob_proto->header);
    }

    FOR_EACH(mob, mob_list) {
        mark_entity(&mob->header);
        mark_list(&mob->objects);
    }
}

static void mark_roots()
{
    for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
        mark_value(*slot);
    }

    for (int i = 0; i < vm.frame_count; i++) {
        mark_object((Obj*)vm.frames[i].closure);
    }

    for (ObjUpvalue* upvalue = vm.open_upvalues;
        upvalue != NULL;
        upvalue = upvalue->next) {
        mark_object((Obj*)upvalue);
    }

    mark_table(&vm.globals);
    mark_compiler_roots();
    mark_object((Obj*)vm.init_string);
    mark_natives();
}

static void trace_references()
{
    while (vm.gray_count > 0) {
        Obj* object = vm.gray_stack[--vm.gray_count];
        blacken_object(object);
    }
}

static void sweep()
{
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->is_marked) {
            object->is_marked = false;
            previous = object;
            object = object->next;
        }
        else {
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            }
            else {
                vm.objects = object;
            }

            free_obj_value(unreached);
        }
    }
}

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
    lox_printf("-- gc begin\n");
    size_t before = vm.bytes_allocated;
#endif

    mark_roots();
    trace_references();
    table_remove_white(&vm.strings);
    sweep();

    vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    lox_printf("-- gc end\n");
    lox_printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
        before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif
}

void free_objects()
{
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        free_obj_value(object);
        object = next;
    }

    free(vm.gray_stack);
}
