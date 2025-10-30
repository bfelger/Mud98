////////////////////////////////////////////////////////////////////////////////
// entity.c
////////////////////////////////////////////////////////////////////////////////

#include "entity.h"

#include <db.h>

#include <lox/list.h>
#include <lox/vm.h>

void init_header(Entity* header, ObjType type)
{
    header->obj.type = type;
    init_table(&header->fields);
    init_list(&header->events);

    header->name = lox_empty_string;

    header->event_triggers = 0;
    header->klass = NULL;
    header->script = 0;

    SET_LOX_FIELD(header, header->name, name);

    SET_NATIVE_FIELD(header, header->vnum, vnum, I32);
}

ObjClass* create_entity_class(Entity* entity, const char* name, const char* bare_class_source)
{
    // First, create a class wrapper for bare_class_source using the name.
    INIT_BUF(buf, MAX_STRING_LENGTH);

    addf_buf(buf, "class %s {\n%s\n}\n", name, bare_class_source);

    compile_context.this_ = entity;

    int result = interpret_code(buf->string);

    free_buf(buf);

    if (result == INTERPRET_COMPILE_ERROR) return NULL;
    if (result == INTERPRET_RUNTIME_ERROR) return NULL;

    // Now look up the class we just created and make sure it exists.
    ObjString* class_name = copy_string(name, (int)strlen(name));
    Value value;
    if (!table_get(&vm.globals, class_name, &value)) {
        runtime_error("Failed to create '%s'.", class_name->chars);
        return NULL;
    }

    if (!IS_CLASS(value)) {
        runtime_error("'%s' is not a class.", class_name->chars);
        return NULL;
    }

    compile_context.this_ = NULL;

    return AS_CLASS(value);
}

static Value is_obj_type_lox(Value value, ObjType obj_type, const char* name,
    int arg_count)
{
    if (arg_count != 0) {
        runtime_error("%s() takes no arguments.", name);
        return FALSE_VAL;
    }

    if (is_obj_type(value, obj_type))
        return TRUE_VAL;

    return FALSE_VAL;
}

Value is_obj_lox(Value receiver, int arg_count, Value* args)
{
    return is_obj_type_lox(receiver, OBJ_OBJ, "is_obj", arg_count);
}

Value is_obj_proto_lox(Value receiver, int arg_count, Value* args)
{
    return is_obj_type_lox(receiver, OBJ_OBJ_PROTO, "is_obj_proto", arg_count);
}

Value is_mob_lox(Value receiver, int arg_count, Value* args)
{
    return is_obj_type_lox(receiver, OBJ_MOB, "is_mob", arg_count);
}

Value is_mob_proto_lox(Value receiver, int arg_count, Value* args)
{
    return is_obj_type_lox(receiver, OBJ_MOB_PROTO, "is_mob_proto", arg_count);
}

Value is_room_lox(Value receiver, int arg_count, Value* args)
{
    return is_obj_type_lox(receiver, OBJ_ROOM, "is_room", arg_count);
}

Value is_room_data_lox(Value receiver, int arg_count, Value* args)
{
    return is_obj_type_lox(receiver, OBJ_ROOM_DATA, "is_room_data", arg_count);
}

Value is_area_lox(Value receiver, int arg_count, Value* args)
{
    return is_obj_type_lox(receiver, OBJ_AREA, "is_area", arg_count);
}

Value is_area_data_lox(Value receiver, int arg_count, Value* args)
{
    return is_obj_type_lox(receiver, OBJ_AREA_DATA, "is_area_data", arg_count);
}

