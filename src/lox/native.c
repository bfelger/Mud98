////////////////////////////////////////////////////////////////////////////////
// native.h
// Extra native functions for Lox
////////////////////////////////////////////////////////////////////////////////

#include "compiler.h"
#include "native.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#include <data/damage.h>

#include <entities/faction.h>

#include <db.h>

extern Table global_const_table;

ObjClass* find_class(const char* class_name)
{
    ObjString* name = copy_string(class_name, (int)strlen(class_name));
    Value value;
    if (!table_get(&vm.globals, name, &value)) {
        runtime_error("Undefined class '%s'.", name->chars);
        exit(70);
    }

    if (!IS_CLASS(value)) {
        runtime_error("'%s' is not a class.", name->chars);
        exit(70);
    }

    return AS_CLASS(value);
}

static void define_native(const char* name, NativeFn function)
{
    push(OBJ_VAL(copy_string(name, (int)strlen(name))));
    push(OBJ_VAL(new_native(function)));
    table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

static void define_native_method(const char* name, NativeMethod method)
{
    push(OBJ_VAL(copy_string(name, (int)strlen(name))));
    push(OBJ_VAL(new_native_method(method)));
    table_set(&native_methods, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void add_global(const char* name, Value val)
{
    push(val);
    Value key = OBJ_VAL(copy_string(name, (int)strlen(name)));
    push(key);
    table_set(&vm.globals, AS_STRING(key), val);
    pop();
    pop();
}

void init_const_natives()
{
    init_table(&native_methods);

    for (int i = 0; native_func_entries[i].name != NULL; ++i)
        define_native(native_func_entries[i].name, native_func_entries[i].func);

    for (int i = 0; native_method_entries[i].name != NULL; ++i)
        define_native_method(native_method_entries[i].name, 
            native_method_entries[i].method);

    init_damage_consts();
    init_faction_consts();
}

void init_world_natives()
{
    add_global("global_areas", OBJ_VAL(&global_areas));
    add_global("global_objs", OBJ_VAL(&obj_list));
    add_global("global_mobs", OBJ_VAL(&mob_list));
}
