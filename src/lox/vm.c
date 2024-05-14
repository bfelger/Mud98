////////////////////////////////////////////////////////////////////////////////
// vm.c
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lox/common.h"
#include "lox/compiler.h"
#include "lox/debug.h"
#include "lox/object.h"
#include "lox/memory.h"
#include "lox/native.h"
#include "lox/vm.h"

#include "comm.h"
#include "config.h"

#define INIT_GC_THRESH  1024ULL * 1024ULL

VM vm;

static void reset_stack()
{
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
    vm.open_upvalues = NULL;
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

    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        pos += sprintf(pos, "[line %d] in ",
            function->chunk.lines[instruction]);
        if (function->name == NULL) {
            pos += sprintf(pos, "script\n");
        }
        else {
            pos += sprintf(pos, "%s()\n", function->name->chars);
        }
    }

    lox_printf("%s", errbuf);

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

    vm.init_string = NULL;
    vm.init_string = copy_string("init", 4);
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

static Value peek(int distance)
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

    if (vm.frame_count == FRAMES_MAX) {
        runtime_error("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frame_count++];
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

static bool invoke(ObjString* name, int arg_count)
{
    Value receiver = peek(arg_count);

    if (!IS_INSTANCE(receiver)) {
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
    ObjClass* klass = AS_CLASS(peek(1));
    table_set(&klass->methods, name, method);
    pop();
}

static bool is_falsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate()
{
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = take_string(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
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
        runtime_error("Operands must be numbers."); \
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
        runtime_error("Operands must be numbers."); \
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
#ifdef DEBUG_TRACE_EXECUTION
        lox_printf("          ");
        for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
            lox_printf("[ ");
            print_value(*slot);
            lox_printf(" ]");
        }
        lox_printf("\n\r");
        disassemble_instruction(&frame->closure->function->chunk,
            (int)(frame->ip - frame->closure->function->chunk.code));
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
                push(frame->slots[slot]);
                break;
            }
        case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
        case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!table_get(&vm.globals, name, &value)) {
                    runtime_error("Undefined variable '%s'.", name->chars);
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
                    runtime_error("Undefined variable '%s'.", name->chars);
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
                if (IS_ARRAY(peek(0))) {
                    ValueArray* array_ = AS_ARRAY(peek(0));
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

                if (IS_TABLE(peek(0))) {
                    Table* table = AS_TABLE(peek(0));
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

                Value comp = peek(0);

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
                    EntityHeader* entity = AS_ENTITY(comp);
                    Value value;
                    if (!table_get(&entity->fields, name, &value)) {
                        runtime_error("Entity '%s' has no field '%s'.", 
                            entity->name->chars, name->chars);
                        pop(); // Entity.
                        return INTERPRET_RUNTIME_ERROR;
                    }

                    pop(); // Entity.

                    // Auto-unmarshal
                    if (IS_RAW_PTR(value)) {
                        push(marshal_raw_ptr(AS_RAW_PTR(value)));
                        break;
                    }

                    push(value);
                    break;
                }
                break;
            }
        case OP_SET_PROPERTY: {
                Value comp = peek(1);
                ObjString* field = READ_STRING();

                if (!IS_INSTANCE(comp) && !IS_ENTITY(comp)) {
                    runtime_error("Only instances and entities can have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (IS_INSTANCE(comp)) {
                    ObjInstance* instance = AS_INSTANCE(comp);
                    table_set(&instance->fields, field, peek(0));
                }
                else {
                    EntityHeader* entity = AS_ENTITY(comp);
                    Value current_value;
                    if (!table_get(&entity->fields, field, &current_value)) {
                        runtime_error("Entity '%s' has no field '%s'.",
                            entity->name->chars, field->chars);
                        Value value = pop();
                        pop(); // Entity.
                        push(value);
                        return INTERPRET_RUNTIME_ERROR;
                    }

                    if (IS_RAW_PTR(current_value)) {
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
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
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
                if (is_falsey(peek(0))) frame->ip += offset;
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
                vm.frame_count--;
                if (vm.frame_count == 0) {
                    pop();

#ifdef DEBUG_TRACE_EXECUTION
                    lox_printf("          ");
                    for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
                        lox_printf("[ ");
                        print_value(*slot);
                        lox_printf(" ]");
                    }
                    lox_printf("\n");
#endif
                    return INTERPRET_OK;
                }

                vm.stack_top = frame->slots;
                push(result);
                frame = &vm.frames[vm.frame_count - 1];

#ifdef DEBUG_TRACE_EXECUTION
                lox_printf("          ");
                for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
                    lox_printf("[ ");
                    print_value(*slot);
                    lox_printf(" ]");
                }
                lox_printf("\n");
#endif
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
    ObjFunction* function = compile(source);
    if (function == NULL) 
        return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    ObjClosure* closure = new_closure(function);
    pop();
    push(OBJ_VAL(closure));
    call_closure(closure, 0);

    InterpretResult res = run();

    return res;
}

InterpretResult call_function(const char* fn_name, int count, ...)
{
    va_list args;

    ObjString* name = copy_string(fn_name, (int)strlen(fn_name));
    Value value;
    if (!table_get(&vm.globals, name, &value)) {
        runtime_error("Undefined variable '%s'.", name->chars);
        exit(70);
    }

    if (!IS_CLOSURE(value)) {
        runtime_error("'%s' is not a callable object.", name->chars);
        exit(70);
    }

    push(OBJ_VAL(value));
    ObjClosure* closure = AS_CLOSURE(value);

    va_start(args, count);

    for (int i = 0; i < count; ++i)
        push(va_arg(args, Value));

    va_end(args);

    call_closure(closure, count);

    InterpretResult rc = run();

    // This is the top-level stack frame; the result and last operand have
    // already been popped, and no return value is possible.
    vm.stack_top -= count; 

    return rc;
}