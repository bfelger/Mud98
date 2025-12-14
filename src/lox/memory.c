////////////////////////////////////////////////////////////////////////////////
// memory.c
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#include <merc.h>

#include "compiler.h"
#include "enum.h"
#include "memory.h"
#include "native.h"
#include "vm.h"

#include <entities/area.h>
#include <entities/event.h>
#include <entities/obj_prototype.h>
#include <entities/mobile.h>
#include <entities/room.h>
#include <entities/faction.h>

#ifdef DEBUG_LOG_GC
#include "debug.h"
#include <stdio.h>
#endif

#include <stdlib.h>
#include <string.h>

#ifdef COUNT_GCS
uint64_t gc_count = 0;
#endif

#define GC_HEAP_GROW_FACTOR 2

extern char* string_space;
extern char* top_string;
extern char str_empty[1];

extern Table global_const_table;

extern bool fBootDb;

// From unit tests
extern Value test_output_buffer;
extern ValueArray* mocks_;

extern ValueArray gc_protect_vals;

void* reallocate(void* pointer, size_t old_size, size_t new_size)
{
    if (new_size == old_size)
        return pointer;

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

// Only for use outside of Lox VM managed memory
void* reallocate_nogc(void* pointer, size_t old_size, size_t new_size)
{
    if (new_size == old_size)
        return pointer;

    if (new_size == 0 && old_size > 0) {
        free_mem(pointer, old_size);
        return NULL;
    }

    char* result = (char*)alloc_mem(new_size);
    if (result == NULL) {
        bug("lox/memory/reallocate_nogc(): alloc_mem() failed.");
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
    if (object->mark_id == vm.current_gc_mark)
        return;
#ifdef DEBUG_LOG_GC
    lox_printf("%p mark ", (void*)object);
    print_value(OBJ_VAL(object));
    lox_printf("\n");
#endif
    object->mark_id = vm.current_gc_mark;

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

static void mark_array(ValueArray* array)
{
    if (array == NULL)
        return;

    mark_object((Obj*)array);
    for (int i = 0; i < array->count; i++) {
        mark_value(array->values[i]);
    }
}

void mark_value(Value value)
{
    if (IS_OBJ(value))
        mark_object(AS_OBJ(value));
}

static void mark_entity(Entity* entity)
{
    if (entity == NULL)
        return;

    mark_object((Obj*)entity->name);
    mark_table(&entity->fields);
    mark_list(&entity->events);
    if (entity->klass)
        mark_object((Obj*)entity->klass);
    if (entity->script)
        mark_object((Obj*)entity->script);
}

static void blacken_object(Obj* object)
{
    if (object == NULL)
        return;

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
    case OBJ_NATIVE_METHOD:
    case OBJ_NATIVE_CMD:
        break;
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
    case OBJ_ENUM: {
        ObjEnum* enum_obj = (ObjEnum*)object;
        mark_object((Obj*)enum_obj->name);
        mark_table(&enum_obj->values);
        break;
    }
    case OBJ_NATIVE:
    case OBJ_RAW_PTR:
    case OBJ_STRING:
        break;
    //
    case OBJ_EVENT: {
        Event* event = (Event*)object;
        mark_value(event->criteria);
        mark_object((Obj*)event->method_name);
        break;
    }
    //
    case OBJ_AREA: {
        Area* area = (Area*)object;
        mark_entity(&area->header);
        mark_table(&area->rooms);
        mark_value(OBJ_VAL(area->data));  // Keep AreaData alive
        break;
    }
    case OBJ_AREA_DATA: {
        AreaData* area_data = (AreaData*)object;
        mark_entity(&area_data->header);
        mark_list(&area_data->instances);
        break;
    }
    case OBJ_ROOM: {
        Room* room = (Room*)object;
        mark_entity(&room->header);
        mark_list(&room->mobiles);
        mark_list(&room->objects);
        mark_value(OBJ_VAL(room->data));  // Keep RoomData alive
        mark_value(OBJ_VAL(room->area));  // Keep Area alive
        // Note: room->exit[] are RoomExit* (not Obj types), managed by memory pool
        break;
    }
    case OBJ_ROOM_DATA: {
        RoomData* room_data = (RoomData*)object;
        mark_entity(&room_data->header);
        mark_list(&room_data->instances);
        break;
    }
    case OBJ_OBJ: {
        Object* obj = (Object*)object;
        mark_entity(&obj->header);
        mark_object((Obj*)obj->owner);
        mark_list(&obj->objects);
        mark_value(OBJ_VAL(obj->prototype));  // Keep ObjPrototype alive
        if (obj->in_room)
            mark_value(OBJ_VAL(obj->in_room));  // Keep Room alive
        break;
    }
    case OBJ_OBJ_PROTO: {
        ObjPrototype* obj_proto = (ObjPrototype*)object;
        mark_entity(&obj_proto->header);
        mark_value(OBJ_VAL(obj_proto->area));  // Keep AreaData alive
        break;
    }
    case OBJ_MOB: {
        Mobile* mob = (Mobile*)object;
        mark_entity(&mob->header);
        mark_list(&mob->objects);
        mark_value(OBJ_VAL(mob->prototype));  // Keep MobPrototype alive
        if (mob->in_room)
            mark_value(OBJ_VAL(mob->in_room));  // Keep Room alive
        break;
    }
    case OBJ_MOB_PROTO: {
        MobPrototype* mob_proto = (MobPrototype*)object;
        mark_entity(&mob_proto->header);
        mark_value(OBJ_VAL(mob_proto->area));  // Keep AreaData alive
        break;
    }
    case OBJ_FACTION: {
        Faction* faction = (Faction*)object;
        mark_entity(&faction->header);
        mark_array(&faction->allies);
        mark_array(&faction->enemies);
        break;
    }
    } // end switch
}

static void free_obj_value(Obj* object)
{
    if (object == NULL)
        return;

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
    case OBJ_NATIVE_METHOD:
        FREE(ObjNativeMethod, object);
        break;
    case OBJ_NATIVE_CMD:
        FREE(ObjNativeCmd, object);
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
            if (!IS_PERM_STRING(str))
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
    case OBJ_ENUM:
        ObjEnum* enum_obj = (ObjEnum*)object;
        free_table(&enum_obj->values);
        FREE(ObjEnum, object);
        break;
    //
    case OBJ_EVENT:
        free_event((Event*)object);
        break;
    //
    case OBJ_AREA:
    case OBJ_AREA_DATA:
    case OBJ_ROOM:
    case OBJ_ROOM_DATA:
    case OBJ_OBJ:
    case OBJ_OBJ_PROTO:
    case OBJ_MOB:
    case OBJ_MOB_PROTO:
    case OBJ_FACTION:
        break;
    } // end switch
}

static void mark_natives()
{
    // Some mobs are attached to descriptors stil in nanny().
    for (Descriptor* desc = descriptor_list; desc != NULL; desc = desc->next) {
        Mobile* mob = desc->character;
        if (mob != NULL)
            mark_object((Obj*)mob);
    }

    mark_table(&native_methods);
    mark_table(&native_cmds);
    mark_table(&native_mob_cmds);
}

static void mark_roots()
{
    for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
        mark_value(*slot);
    }

    for (int i = 0; i < vm.frame_count; i++) {
        mark_object((Obj*)vm.frames[i].closure);
    }

    for (ObjUpvalue* upvalue = vm.open_upvalues; upvalue != NULL; 
            upvalue = upvalue->next) {
        mark_object((Obj*)upvalue);
    }

    mark_table(&vm.globals);
    mark_table(&global_const_table);
    mark_compiler_roots();
    mark_object((Obj*)vm.init_string);
    mark_object((Obj*)lox_empty_string);
    mark_natives();

    mark_value(repl_ret_val);

    // Mark objects needed for unit tests
    mark_value(test_output_buffer);
    mark_array(mocks_);

    // Temp space for Entity Values we need to preserve during construction
    mark_array(&gc_protect_vals);
    mark_table(&faction_table);
    mark_array(&global_areas);
    mark_global_rooms();
    mark_global_mob_protos();
    mark_global_obj_protos();

    mark_list(&mob_free);
    mark_list(&mob_list);
    mark_list(&obj_free);
    mark_list(&obj_free);

    EventTimer* timer = event_timers;
    while (timer != NULL) {
        mark_object((Obj*)timer->closure);
        timer = timer->next;
    }
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
        if (object->mark_id == vm.current_gc_mark) {
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
    // Prevent reentrancy - diagnostic logging allocates memory!
    if (vm.gc_running) {
        return;
    }
    vm.gc_running = true;

#if defined(DEBUG_LOG_GC) || defined(COUNT_GCS)
    lox_printf("-- gc begin\n");
    size_t before = vm.bytes_allocated;
#endif

#ifdef COUNT_GCS
    gc_count++;
#endif

    // We don't add game entities to VM globals until after boot. Don't GC
    // anything until we've had a chance to do that.
    if (!fBootDb) {
        vm.current_gc_mark++;
        mark_roots();
        trace_references();
        table_remove_white(&vm.strings);
        sweep();
    }

    vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;

#if defined(DEBUG_LOG_GC) || defined(COUNT_GCS)
    lox_printf("-- gc end\n");
    lox_printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
        before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif

    vm.gc_running = false;
}

void collect_garbage_nongrowing()
{
    // Prevent reentrancy
    if (vm.gc_running) {
        return;
    }
    vm.gc_running = true;

#if defined(DEBUG_LOG_GC) || defined(COUNT_GCS)
    lox_printf("-- gc (non-growing) begin\n");
    size_t before = vm.bytes_allocated;
#endif

#ifdef COUNT_GCS
    gc_count++;
#endif

    // We don't add game entities to VM globals until after boot. Don't GC
    // anything until we've had a chance to do that.
    if (!fBootDb) {
        vm.current_gc_mark++;
        mark_roots();
        trace_references();
        table_remove_white(&vm.strings);
        sweep();
    }

#if defined(DEBUG_LOG_GC) || defined(COUNT_GCS)
    lox_printf("-- gc (non-growing) end\n");
    lox_printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
        before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif

    vm.gc_running = false;
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
