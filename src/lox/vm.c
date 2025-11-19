////////////////////////////////////////////////////////////////////////////////
// vm.c
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "enum.h"
#include "object.h"
#include "memory.h"
#include "native.h"
#include "vm.h"

#include <comm.h>
#include <config.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define INIT_GC_THRESH  1024ULL * 1024ULL

VM vm;
ObjString* lox_empty_string = NULL;
Value repl_ret_val = NIL_VAL;

Table global_const_table;

// We need to protect some Values from garbage collection while they are being
// built (such as Entity values, like ObjString names and somesuch). Usually
// we would push them on the stack, but with Lox Classes being built as part of
// Entity creation, we probably don't want to pollute it. This is a private
// reserve of Values we want to protect. When the Entity is built and added to
// the relevant GC-observable container, the entire array can be cleared.
ValueArray gc_protect_vals;

void gc_protect(Value value)
{
    write_value_array(&gc_protect_vals, value);
}

void gc_protect_clear()
{
    for (int i = 0; i < gc_protect_vals.count; i++) {
        gc_protect_vals.values[i] = NIL_VAL;
    }

    gc_protect_vals.count = 0;
}

extern bool test_disassemble_on_error;
extern bool test_disassemble_on_test;
extern bool test_trace_exec;
extern bool test_output_enabled;

//#define DEBUG_FRAME_COUNT

#ifdef DEBUG_FRAME_COUNT
#define INCREMENT_FRAME_COUNT() increment_frame_count()
#define DECREMENT_FRAME_COUNT() decrement_frame_count()
static inline int increment_frame_count()
{
    if (vm.frame_count + 1 > FRAMES_MAX) {
        runtime_error("Stack overflow.");
        exit(1);
    }
    return vm.frame_count++;
}
static inline int decrement_frame_count()
{
    if (vm.frame_count - 1 < 0) {
        runtime_error("Stack underflow.");
        exit(1);
    }
    return vm.frame_count--;
}
#else
#define INCREMENT_FRAME_COUNT() (vm.frame_count++)
#define DECREMENT_FRAME_COUNT() (vm.frame_count--)
#endif

void reset_stack()
{
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
    vm.open_upvalues = NULL;
}

void print_stack()
{
    lox_printf("Stack:    ");
    for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
        lox_printf("[ ");
        print_value(*slot);
        lox_printf(" ]");
    }
    lox_printf("\n\r");
}

void runtime_error(const char* format, ...)
{
    char errbuf[1024] = "";
    char* pos = errbuf;

    va_list args;
    va_start(args, format);
    pos += vsprintf(pos, format, args);
    va_end(args);
    pos += sprintf(pos, "\n");

    int fc = vm.frame_count;

    if (fc > FRAMES_MAX || fc < 0) {
        fc = FRAMES_MAX;
        fprintf(stderr, "Too many call frames to display (%d).\n", vm.frame_count);
    }

    for (int i = fc - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        if (frame->closure == NULL)
            continue;
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        pos += sprintf(pos, "Lox error [line %d] in ",
            function->chunk.lines[instruction]);
        if (function->name == NULL) {
            pos += sprintf(pos, "script\n");
        }
        else {
            pos += sprintf(pos, "%s()\n", function->name->chars);
        }
    }

    //bool test_output = test_output_enabled;
    //test_output_enabled = false;
    lox_printf("%s", errbuf);
    //test_output_enabled = test_output;

    print_stack();

    reset_stack();
}

void init_vm()
{
    reset_stack();
    vm.objects = NULL;
    vm.bytes_allocated = 0;
    vm.next_gc = INIT_GC_THRESH;

    vm.gray_count = 0;
    vm.gray_capacity = 0;
    vm.gray_stack = NULL;

    init_table(&vm.globals);
    init_table(&vm.strings);
    init_table(&global_const_table);

    char* end_str = alloc_perm(1);
    end_str[0] = '\0';
    lox_empty_string = copy_string(end_str, 1);

    char* str_init = alloc_perm(5);
    strcpy(str_init, "init");
    vm.init_string = copy_string(str_init, 4);

    init_value_array(&gc_protect_vals);
}

void free_vm()
{
    free_table(&vm.globals);
    free_table(&vm.strings);
    vm.init_string = NULL;
    free_objects();
}

void push(Value value)
{
    *vm.stack_top = value;
    vm.stack_top++;
}

Value pop()
{
    vm.stack_top--;
    return *vm.stack_top;
}

Value peek(int distance)
{
    return vm.stack_top[-1 - distance];
}

bool call_closure(ObjClosure* closure, int arg_count)
{
    if (arg_count != closure->function->arity) {
        runtime_error("Expected %d arguments but got %d.", 
            closure->function->arity, arg_count);
        return false;
    }

    if (vm.frame_count >= FRAMES_MAX || vm.frame_count < 0) {
        runtime_error("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[INCREMENT_FRAME_COUNT()];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stack_top - arg_count - 1;
    return true;
}

bool call_value(Value callee, int arg_count)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm.stack_top[-arg_count - 1] = bound->receiver;
                return call_closure(bound->method, arg_count);
            }
        case OBJ_CLASS: {
                ObjClass* klass = AS_CLASS(callee);
                vm.stack_top[-arg_count - 1] = OBJ_VAL(new_instance(klass));
                Value initializer;
                if (table_get(&klass->methods, vm.init_string, &initializer)) {
                    return call_closure(AS_CLOSURE(initializer), arg_count);
                }
                else if (arg_count != 0) {
                    runtime_error("Expected 0 arguments but got %d.", arg_count);
                    return false;
                }
                return true;
            }
        case OBJ_CLOSURE:
            return call_closure(AS_CLOSURE(callee), arg_count);
        case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(arg_count, vm.stack_top - arg_count);
                vm.stack_top -= (ptrdiff_t)arg_count + 1;
                push(result);
                return true;
            }
        case OBJ_NATIVE_CMD: {
                if (arg_count != 1 || !IS_STRING(peek(0))) {
                    runtime_error("Native commands only take a string argument.");
                    return false;
                }
                DoFunc* native = AS_NATIVE_CMD(callee)->native;
                Mobile* self = exec_context.me;
                if (self == NULL) {
                    runtime_error("No valid execution context.");
                    return false;
                }
                ObjString* str_arg = AS_STRING(peek(0));
                char* arg = str_dup(str_arg->chars);
                (*native)(self, arg);
                free_string(arg);
                vm.stack_top -= (ptrdiff_t)arg_count + 1;
                push(NIL_VAL);
                return true;
            }
        default:
            break; // Non-callable object type.
        } // end switch
    }
    runtime_error("Can only call functions and classes.");
    return false;
}

static bool invoke_from_class(ObjClass* klass, ObjString* name, int arg_count)
{
    Value method;
    if (!table_get(&klass->methods, name, &method)) {
        runtime_error("Undefined property '%s'.", name->chars);
        return false;
    }
    return call_closure(AS_CLOSURE(method), arg_count);
}

bool invoke(ObjString* name, int arg_count)
{
    Value receiver = peek(arg_count);

    if (IS_ENTITY(receiver)) {
        // Check to see if the name is in the native methods table.
        Value method;
        if (table_get(&native_methods, name, &method)) {
            // Found it. Insert the entity as the receiver and call it.
            vm.stack_top[-arg_count - 1] = receiver;
            NativeMethod native = AS_NATIVE_METHOD(method)->native;
            Value result = native(receiver, arg_count, vm.stack_top - arg_count);
            vm.stack_top -= (ptrdiff_t)arg_count + 1;
            push(result);
            return true;
        }

        Entity* entity = AS_ENTITY(receiver);
        if (table_get(&entity->fields, name, &method)) {
            vm.stack_top[-arg_count - 1] = method;
            return call_value(method, arg_count);
        }

        if (IS_MOBILE(receiver) && arg_count == 1 && IS_STRING(peek(0))) {
            Mobile* mob = AS_MOBILE(receiver);
            if (table_get(&native_cmds, name, &method) || 
                (mob->pcdata == NULL &&
                    table_get(&native_mob_cmds, name, &method))) {
                // Mobs have special native functions and methods for in-game
                // commands.
                vm.stack_top[-arg_count - 1] = receiver;
                DoFunc* native = AS_NATIVE_CMD(method)->native;
                ObjString* str_arg = AS_STRING(peek(0));
                char* arg = str_dup(str_arg->chars);
                (*native)(mob, arg);
                free_string(arg);
                vm.stack_top -= (ptrdiff_t)arg_count + 1;
                push(NIL_VAL);
                return true;
            }
        }

        Value value;
        if (table_get(&entity->fields, name, &value)) {
            vm.stack_top[-arg_count - 1] = value;
            return call_value(value, arg_count);
        }

        if (entity->klass == NULL) {
            runtime_error("Entity has no class.");
            return false;
        }

        return invoke_from_class(entity->klass, name, arg_count);
    }

    if (IS_ARRAY(receiver)) {
        if (!strcmp(name->chars, "add")) {
            ValueArray* array = AS_ARRAY(receiver);
            for (int i = 0; i < arg_count; i++) {
                Value value = peek(arg_count - i - 1);
                write_value_array(array, value);
            }
            vm.stack_top -= (ptrdiff_t)arg_count + 1;
            push(NIL_VAL);
            return true;
        }
        else if (!strcmp(name->chars, "contains")) {
            if (arg_count != 1) {
                runtime_error("'contains()' takes only one argument.");
                return false;
            }
            ValueArray* array = AS_ARRAY(receiver);
            Value ret = value_array_contains(array, peek(0)) ? TRUE_VAL : FALSE_VAL;            
            vm.stack_top -= (ptrdiff_t)arg_count + 1;
            push(ret);
            return true;
        }
    }

    if (!IS_INSTANCE(receiver)) {
        lox_printf("Receiver: ");
        print_value(receiver);
        lox_printf("\n");
        runtime_error("Only instances have methods.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    Value value;
    if (table_get(&instance->fields, name, &value)) {
        vm.stack_top[-arg_count - 1] = value;
        return call_value(value, arg_count);
    }

    return invoke_from_class(instance->klass, name, arg_count);
}

static bool bind_method(ObjClass* klass, ObjString* name)
{
    Value method;
    if (!table_get(&klass->methods, name, &method)) {
        runtime_error("Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = new_bound_method(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));
    return true;
}

static ObjUpvalue* capture_upvalue(Value* local)
{
    ObjUpvalue* prev_upvalue = NULL;
    ObjUpvalue* upvalue = vm.open_upvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* created_upvalue = new_upvalue(local);
    created_upvalue->next = upvalue;

    if (prev_upvalue == NULL) {
        vm.open_upvalues = created_upvalue;
    }
    else {
        prev_upvalue->next = created_upvalue;
    }

    return created_upvalue;
}

static void close_upvalues(Value* last)
{
    while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
        ObjUpvalue* upvalue = vm.open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.open_upvalues = upvalue->next;
    }
}

static void define_method(ObjString* name)
{
    Value method = peek(0);
    if (IS_CLASS(peek(1))) {
        ObjClass* klass = AS_CLASS(peek(1));
        table_set(&klass->methods, name, method);
    }
    else if (IS_ENTITY(peek(1))) {
        Entity* entity = AS_ENTITY(peek(1));
        table_set(&entity->fields, name, method);
    }
    pop();
}

static bool is_falsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate()
{
    ObjString* b = NULL;
    char* b_str = NULL;
    int b_len;
    if (IS_STRING(peek(0))) {
        b = AS_STRING(peek(0));
        b_str = b->chars;
        b_len = b->length;
    }
    else {
        b_str = str_dup(string_value(peek(0)));
        b_len = (int)strlen(b_str);
    }

    ObjString* a = NULL;
    char* a_str = NULL;
    int a_len;
    if (IS_STRING(peek(1))) {
        a = AS_STRING(peek(1));
        a_str = a->chars;
        a_len = a->length;
    }
    else {
        a_str = str_dup(string_value(peek(1)));
        a_len = (int)strlen(a_str);
    }

    int length = a_len + b_len;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a_str, a_len);
    memcpy(chars + a_len, b_str, b_len);
    chars[length] = '\0';

    ObjString* result = take_string(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
}

static void interpolate_string(int count)
{
    INIT_BUF(out, MAX_STRING_LENGTH);

    char* pos = out->string;

    for (int i = count - 1; i >= 0; i--) {
        Value val = peek(i);
        char* str;
        if (IS_STRING(val))
            str = AS_STRING(val)->chars;
        else
            str = string_value(val);
        int len = (int)strlen(str);

        if (len > MAX_STRING_LENGTH) {
            runtime_error("string_interp: Max string length exceeded.");
            break;
        }

        memcpy(pos, str, len);
        pos += len;
    }
    *pos = '\0';

    ObjString* result = copy_string(out->string, (int)(pos - out->string));

    vm.stack_top -= count;

    push(OBJ_VAL(result));

    free_buf(out);
}

static bool each_prime(Value callee, Value collection)
{
    if (IS_ARRAY(collection)) {
        push(INT_VAL(0));
    }
    else if (IS_TABLE(collection)) {
        Table* table = AS_TABLE(collection);
        int i = 0;
        for (i = 0; i < table->capacity; i++) {
            Entry* entry = &table->entries[i];
            if (entry->key != NIL_VAL) {
                break;
            };
        }
        push(INT_VAL(i));
    }
    else if (IS_LIST(collection)) {
        List* list = AS_LIST(collection);
        Node* node = list->front;
        // Value is a uint64_t, which is sufficient to hold a pointer to Node.
        // Note that this value cannot be printed. Take great care.
        // Making a RAW_U64 would be safer, but slower.
        push((Value)node);
    }
    else {
        runtime_error("each_prime: Cannot call each() on this type.");
        return false;
    }

    return true;
}

static bool each_end_list()
{
    Value callee = peek(1);
    Node* node = (Node*)peek(0);

    if (node == NULL) {
        pop(); // Callee
        pop(); // Collection
        return false;
    }

    push(callee);
    push(node->value);

    return call_value(callee, 1);
}

static bool each_end_table()
{
    Table* table = AS_TABLE(peek(2));
    Value callee = peek(1);
    int32_t i = AS_INT(peek(0));

    if (i >= table->capacity) {
        pop(); // Callee
        pop(); // Collection
        return false;
    }

    Entry* entry = &table->entries[i];

    push(callee);
    push(entry->key);
    push(entry->value);

    return call_value(callee, 2);
}

static bool each_end_array()
{
    ValueArray* array_ = AS_ARRAY(peek(2));
    Value callee = peek(1);
    int32_t i = AS_INT(peek(0));

    if (i >= array_->count) {
        pop(); // Callee
        pop(); // Collection
        return false;
    }

    push(callee);
    push(INT_VAL(i));
    push(array_->values[i]);

    return call_value(callee, 2);
}

static bool each_or_end()
{
    // Stack:
    // 2: Collection
    // 1: Callee
    // 0: Loop Var
    if (IS_ARRAY(peek(2))) {
        return each_end_array();
    }
    else if (IS_TABLE(peek(2))) {
        return each_end_table();
    }
    else if (IS_LIST(peek(2))) {
        return each_end_list();
    }
    else {
        runtime_error("each_or_end: Cannot call each() on this type.");
        return false;
    }
}

static bool each_advance()
{
    if (IS_ARRAY(peek(2))) {
        push(INT_VAL(AS_INT(pop()) + 1));
        return true;
    }
    else if (IS_TABLE(peek(2))) {
        Table* table = AS_TABLE(peek(2));
        int i = 0;
        for (i = AS_INT(pop()) + 1; i < table->capacity; i++) {
            Entry* entry = &table->entries[i];
            if (entry->key != NIL_VAL) {
                break;
            };
        }
        push(INT_VAL(i));
        return true;
    }
    else if (IS_LIST(peek(2))) {
        Node* node = (Node*)pop();
        node = node->next;
        push((Value)node);
        return true;
    }
    else {
        runtime_error("each_advance: Cannot call each() on this type.");
        return false;
    }
}

InterpretResult run()
{
    char err_buf[256] = { 0 };
    CallFrame* frame = &vm.frames[vm.frame_count - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[ \
        *frame->ip & 0x80 ? READ_SHORT() & 0x7fff : READ_BYTE() \
        ])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(op) \
    do { \
      if ((!IS_INT(peek(0)) && !IS_DOUBLE(peek(0))) \
        || (!IS_INT(peek(1)) && !IS_DOUBLE(peek(1)))) { \
        char c = (#op)[0]; \
        runtime_error("Operands for '%c' must be numbers.", c); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      if (IS_DOUBLE(peek(0)) || IS_DOUBLE(peek(1))) { \
        double b = IS_DOUBLE(peek(0)) ? AS_DOUBLE(pop()) : (double)AS_INT(pop()); \
        double a = IS_DOUBLE(peek(0)) ? AS_DOUBLE(pop()) : (double)AS_INT(pop()); \
        push(DOUBLE_VAL(a op b)); \
      } \
      else { \
        int32_t b = AS_INT(pop()); \
        int32_t a = AS_INT(pop()); \
        push(INT_VAL(a op b)); \
      } \
    } while (false)

#define BOOL_OP(op) \
    do { \
      if ((!IS_INT(peek(0)) && !IS_DOUBLE(peek(0))) \
        || (!IS_INT(peek(1)) && !IS_DOUBLE(peek(1)))) { \
        char c = (#op)[0]; \
        runtime_error("Operands for '%c' must be numbers.", c); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      if (IS_DOUBLE(peek(0)) || IS_DOUBLE(peek(1))) { \
        double b = IS_DOUBLE(peek(0)) ? AS_DOUBLE(pop()) : (double)AS_INT(pop()); \
        double a = IS_DOUBLE(peek(0)) ? AS_DOUBLE(pop()) : (double)AS_INT(pop()); \
        push(BOOL_VAL(a op b)); \
      } \
      else { \
        int32_t b = AS_INT(pop()); \
        int32_t a = AS_INT(pop()); \
        push(BOOL_VAL(a op b)); \
      } \
    } while (false)

    for (;;) {
#ifndef DEBUG_TRACE_EXECUTION
        if (test_trace_exec) {
#endif
            print_stack();
            disassemble_instruction(&frame->closure->function->chunk,
                (int)(frame->ip - frame->closure->function->chunk.code));
#ifndef DEBUG_TRACE_EXECUTION
        }
#endif
        uint8_t instruction = READ_BYTE();
        switch (instruction/* = READ_BYTE()*/) {
        case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
        case OP_NIL:        push(NIL_VAL); break;
        case OP_TRUE:       push(BOOL_VAL(true)); break;
        case OP_FALSE:      push(BOOL_VAL(false)); break;
        case OP_POP:        pop(); break;
        case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();

                if (IS_RAW_PTR(frame->slots[slot])) {
                    push(marshal_raw_ptr(AS_RAW_PTR(frame->slots[slot])));
                    break;
                }

                push(frame->slots[slot]);
                break;
            }
        case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();

                if (IS_RAW_PTR(frame->slots[slot])) {
                    ObjRawPtr* raw = AS_RAW_PTR(frame->slots[slot]);
                    unmarshal_raw_val(raw, peek(0));
                }
                else {
                    frame->slots[slot] = peek(0);
                }
                break;
            }
        case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;

                // Check for an in-game (player-driven) execution context and 
                // look up in-game commands. Treat them like native functions.
                if (exec_context.me != NULL) {
                    if (table_get(&native_cmds, name, &value)) {
                        push(value);
                        break;
                    }

                    if (exec_context.me->pcdata == NULL) {
                        if (table_get(&native_mob_cmds, name, &value)) {
                            push(value);
                            break;
                        }
                    }
                }

                if (!table_get(&vm.globals, name, &value)) {
                    runtime_error("Undefined global variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
        case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                table_set(&vm.globals, name, peek(0));
                pop();
                break;
            }
        case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (table_set(&vm.globals, name, peek(0))) {
                    table_delete(&vm.globals, name);
                    runtime_error("Undefined global variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
        case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
        case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
        case OP_GET_PROPERTY: {
                ObjString* name = READ_STRING();
                Value comp = peek(0);

                if (IS_ARRAY(comp)) {
                    ValueArray* array_ = AS_ARRAY(comp);
                    if (!strcmp(name->chars, "count")) {
                        Value count = INT_VAL(array_->count);
                        pop(); // Array
                        push(count);
                        break;
                    }
                    else {
                        runtime_error("Bad array accessor '.%s'.", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }

                if (IS_TABLE(comp)) {
                    Table* table = AS_TABLE(comp);
                    if (!strcmp(name->chars, "count")) {
                        Value count = INT_VAL(table->count);
                        pop(); // Array
                        push(count);
                        break;
                    }
                    else {
                        runtime_error("Bad table accessor '.%s'.", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }

                if (IS_LIST(comp)) {
                    List* list = AS_LIST(comp);
                    if (!strcmp(name->chars, "count")) {
                        Value count = INT_VAL(list->count);
                        pop(); // Array
                        push(count);
                        break;
                    }
                    else {
                        runtime_error("Bad list accessor '.%s'.", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }

                if (IS_ENUM(comp)) {
                    ObjEnum* enum_obj = AS_ENUM(comp);
                    if (!strcmp(name->chars, "count")) {
                        Value count = INT_VAL(enum_obj->values.count);
                        pop();
                        push(count);
                        break;
                    }

                    Value value;
                    if (table_get(&enum_obj->values, name, &value)) {
                        pop();
                        push(value);
                        break;
                    }

                    const char* enum_name = enum_obj->name != NULL
                        ? enum_obj->name->chars : "<enum>";
                    runtime_error("Enum '%s' has no member '%s'.",
                        enum_name, name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (!IS_INSTANCE(comp) && !IS_ENTITY(comp)) {
                    runtime_error("Only instances and entities have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (IS_INSTANCE(comp)) {
                    ObjInstance* instance = AS_INSTANCE(comp);
                    Value value;
                    if (table_get(&instance->fields, name, &value)) {
                        pop(); // Instance.
                        push(value);
                        break;
                    }

                    if (!bind_method(instance->klass, name)) {
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                else {
                    Entity* entity = AS_ENTITY(comp);
                    Value value;

                    if (table_get(&entity->fields, name, &value)) {
                        pop(); // Entity.

                        if (IS_RAW_PTR(value)) {
                            push(marshal_raw_ptr(AS_RAW_PTR(value)));
                            break;
                        }

                        push(value);
                        break;
                    }

                    if (!bind_method(entity->klass, name)) {
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
        case OP_SET_PROPERTY: {
                Value comp = peek(1);
                ObjString* field = READ_STRING();

                if (IS_ENUM(comp)) {
                    ObjEnum* enum_obj = AS_ENUM(comp);
                    const char* enum_name = enum_obj->name != NULL
                        ? enum_obj->name->chars : "<enum>";
                    runtime_error("Cannot assign to enum member '%s.%s'.",
                        enum_name, field->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (!IS_INSTANCE(comp) && !IS_ENTITY(comp)) {
                    print_value(comp);
                    runtime_error("Only instances and entities can have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (IS_INSTANCE(comp)) {
                    ObjInstance* instance = AS_INSTANCE(comp);
                    table_set(&instance->fields, field, peek(0));
                }
                else {
                    Entity* entity = AS_ENTITY(comp);
                    Value current_value;
                    if (table_get(&entity->fields, field, &current_value) && IS_RAW_PTR(current_value)) {
                        ObjRawPtr* raw = AS_RAW_PTR(current_value);
                        unmarshal_raw_val(raw, peek(0));
                    }
                    else {
                        table_set(&entity->fields, field, peek(0));
                    }
                }
                Value value = pop();
                pop();
                push(value);
                break;
            }
        case OP_GET_AT_INDEX: {
                if (!IS_INT(peek(0))) {
                    runtime_error("Array indexes must be integers.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (!IS_ARRAY(peek(1))) {
                    runtime_error("Only arrays can be indexed.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int index = (int)AS_INT(pop());
                ValueArray* val_array = AS_ARRAY(peek(0));
                if (index < 0 || index >= val_array->count) {
                    sprintf(err_buf, "Index %d is out of bounds.", index);
                    runtime_error(err_buf);
                }
                pop();
                push(val_array->values[index]);
                break;
            }
        case OP_SET_AT_INDEX: {
                if (!IS_INT(peek(1))) {
                    runtime_error("Array indexes must be integers.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (!IS_ARRAY(peek(2))) {
                    runtime_error("Only arrays can be indexed.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int32_t index = AS_INT(peek(1));
                ValueArray* val_array = AS_ARRAY(peek(2));
                if (index < 0 || index >= val_array->count) {
                    sprintf(err_buf, "Index %d is out of bounds.", index);
                    runtime_error(err_buf);
                }
                val_array->values[index] = peek(0);
                pop();
                pop();
                break;
            }
        case OP_GET_SUPER: {
                ObjString* name = READ_STRING();
                ObjClass* superclass = AS_CLASS(pop());

                if (!bind_method(superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
        case OP_EQUAL:{
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(values_equal(a, b)));
                break;
            }
        case OP_GREATER:    BOOL_OP(>); break;
        case OP_LESS:       BOOL_OP(<); break;
        case OP_ADD: {
                if (IS_STRING(peek(0)) || IS_STRING(peek(1))) {
                    concatenate();
                }
                else {
                    BINARY_OP(+);
                }
                break;
            }
        case OP_SUBTRACT:   BINARY_OP(-); break;
        case OP_MULTIPLY:   BINARY_OP(*); break;
        case OP_DIVIDE:     BINARY_OP(/); break;
        case OP_NOT:
            push(BOOL_VAL(is_falsey(pop())));
            break;
        case OP_NEGATE:
            if (IS_DOUBLE(peek(0)))
                push(DOUBLE_VAL(-AS_DOUBLE(pop())));
            else if (IS_INT(peek(0)))
                push(INT_VAL(-AS_INT(pop())));
            else {
                runtime_error("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        case OP_PRINT: {
                print_value(pop());
                lox_printf("\n");
                break;
            }
        case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
        case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (is_falsey(peek(0)))
                    frame->ip += offset;
                break;
            }
        case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
        case OP_CALL: {
                int arg_count = READ_BYTE();
                if (!call_value(peek(arg_count), arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
        case OP_ARRAY: {
                ValueArray* array_ = new_obj_array();
                int elem_count = READ_BYTE();
                for (int i = 0; i < elem_count; ++i) {
                    write_value_array(array_, peek((elem_count - i) - 1));
                }
                vm.stack_top -= elem_count;
                push(OBJ_VAL(array_));
                break;
            }
        case OP_INVOKE: {
                ObjString* method = READ_STRING();
                int arg_count = READ_BYTE();
                if (!invoke(method, arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
        case OP_SUPER_INVOKE: {
                ObjString* method = READ_STRING();
                int arg_count = READ_BYTE();
                ObjClass* superclass = AS_CLASS(pop());
                if (!invoke_from_class(superclass, method, arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
        case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = new_closure(function);
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalue_count; i++) {
                    uint8_t is_local = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (is_local) {
                        closure->upvalues[i] = capture_upvalue(frame->slots + 
                            index);
                    }
                    else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
        case OP_CLOSE_UPVALUE:
            close_upvalues(vm.stack_top - 1);
            pop();
            break;
        case OP_RETURN: {
                Value result = pop();
                close_upvalues(frame->slots);
                DECREMENT_FRAME_COUNT();
                if (vm.frame_count == 0) {
                    repl_ret_val = result;
                    pop();
                    return INTERPRET_OK;
                }

                vm.stack_top = frame->slots;
                push(result);
                frame = &vm.frames[vm.frame_count - 1];

                break;
            }
        case OP_CLASS:
            push(OBJ_VAL(new_class(READ_STRING())));
            break;
        case OP_INHERIT: {
                Value superclass = peek(1);
                if (!IS_CLASS(superclass)) {
                    runtime_error("Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjClass* subclass = AS_CLASS(peek(0));
                table_add_all(&AS_CLASS(superclass)->methods, &subclass->methods);
                pop(); // Subclass.
                break;
            }
        case OP_METHOD:
            define_method(READ_STRING());
            break;
        case OP_EACH_PRIME: {
            if (!each_prime(peek(0), peek(1))) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frame_count - 1];
            break;
        }
        case OP_EACH_OR_END: {
            uint16_t offset = READ_SHORT();
            if (!each_or_end())
                frame->ip += offset;
            else
                frame = &vm.frames[vm.frame_count - 1];
            break;
        }
        case OP_EACH_ADVANCE: {
            if (!each_advance()) {
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_INTERP: {
            int count = READ_BYTE();
            interpolate_string(count);
            break;
        }
        // In-game entities
        case OP_SELF:
            if (exec_context.me != NULL)
                push(OBJ_VAL(exec_context.me));
            else
                push(NIL_VAL);
            break;
        } // end switch
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret_code(const char* source)
{
    repl_ret_val = NIL_VAL;

    ObjFunction* function = compile(source);
    if (function == NULL) 
        return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    ObjClosure* closure = new_closure(function);
    pop();

    if (closure == NULL) {
        return INTERPRET_RUNTIME_ERROR;
    }

    push(OBJ_VAL(closure));
    
    if (!call_closure(closure, 0)) {
        reset_stack();
        return INTERPRET_RUNTIME_ERROR;
    }

    InterpretResult res = run();

    if ((res != INTERPRET_OK && test_disassemble_on_error) || test_disassemble_on_test) {
        disassemble_chunk(&function->chunk, "<script>");
    }

    return res;
}

InterpretResult call_function(const char* fn_name, int count, ...)
{
    va_list args;

    ObjString* name = copy_string(fn_name, (int)strlen(fn_name));
    Value value;
    if (!table_get(&vm.globals, name, &value)) {
        runtime_error("Undefined callable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
    }

    if (!IS_CLOSURE(value)) {
        runtime_error("'%s' is not a callable object.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
    }

    push(OBJ_VAL(value));
    ObjClosure* closure = AS_CLOSURE(value);

    va_start(args, count);

    for (int i = 0; i < count; ++i)
        push(va_arg(args, Value));

    va_end(args);

    if (!call_closure(closure, count)) {
        return INTERPRET_RUNTIME_ERROR;
    }

    InterpretResult rc = run();

    // This is the top-level stack frame; the result and last operand have
    // already been popped, and no return value is possible.
    vm.stack_top -= count; 

    return rc;
}

InterpretResult invoke_closure(ObjClosure* closure, int count, ...)
{
    va_list args;

    push(OBJ_VAL(closure));

    va_start(args, count);

    for (int i = 0; i < count; ++i)
        push(va_arg(args, Value));

    va_end(args);

    if (!call_closure(closure, count)) {
        return INTERPRET_RUNTIME_ERROR;
    }

    InterpretResult rc = run();

    // This is the top-level stack frame; the result and last operand have
    // already been popped, and no return value is possible.
    vm.stack_top -= count;

    return rc;
}

void invoke_method_closure(Value receiver, ObjClosure* closure, int count, ...)
{
    reset_stack();

    push(receiver);

    va_list args;

    va_start(args, count);

    for (int i = 0; i < count; ++i)
        push(va_arg(args, Value));

    va_end(args);

    if (!call_closure(closure, count)) {
        bug("invoke_method_closure(): Error calling method closure '%s'.\n", closure->function->name->chars);
        return;
    }

    InterpretResult rc = run();

    if (rc != INTERPRET_OK)
        bugf("invoke_method_closure(): Error invoking method closure '%s'.\n", closure->function->name->chars);

    // This is the top-level stack frame; the result and last operand have
    // already been popped, and no return value is possible.
    vm.stack_top -= count + 1;
}

// Called from entities
void init_entity_class(Entity* entity)
{
    if (entity == NULL || entity->klass == NULL) {
        return;
    }

    Value initializer;
    if (table_get(&entity->klass->methods, vm.init_string, &initializer)) {
        invoke_method_closure(OBJ_VAL(entity), AS_CLOSURE(initializer), 0);
    }
}
