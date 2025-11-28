////////////////////////////////////////////////////////////////////////////////
// compiler.c
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "compiler.h"
#include "enum.h"
#include "memory.h"
#include "native.h"
#include "scanner.h"
#include "table.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#include <recycle.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

extern Table global_const_table;
extern bool test_output_enabled;

void send_to_char(const char* txt, Mobile* ch);

CompileContext compile_context = { 0 };
ExecContext exec_context = { 0 };

typedef struct {
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . () []
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool can_assign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
    bool is_captured;
    bool is_const;
    bool occupies_slot;
    bool has_const_value;
    Value const_value;
    int slot;
} Local;

typedef struct {
    uint8_t index;
    bool is_local;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT,
    TYPE_LAMDA,
} FunctionType;

typedef struct LoopContext {
    struct LoopContext* enclosing;
    int loop_start;
    int exit_jump;
    int exit_patches[UINT8_COUNT];
    int exit_patch_count;
    int scope_depth;
} LoopContext;

typedef struct Compiler {
    struct Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int local_count;
    int slot_count;
    Upvalue upvalues[UINT8_COUNT];
    int scope_depth;
    LoopContext* current_loop;
} Compiler;

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    bool has_superclass;
    Entity* entity_this;
} ClassCompiler;

Parser parser;
Compiler* current = NULL;
ClassCompiler* current_class = NULL;

static Chunk* current_chunk()
{
    return &current->function->chunk;
}

void compile_errorf(const char* fmt, ...)
{
    char errbuf[1024];

    va_list args;
    va_start(args, fmt);
    vsprintf(errbuf, fmt, args);
    va_end(args);

    if (compile_context.me == NULL) {
        fprintf(stderr, "%s", errbuf);
    }
    else {
        send_to_char(errbuf, compile_context.me);
    }
}

static void error_at(Token* token, const char* message)
{
    if (parser.panic_mode) 
        return;
    parser.panic_mode = true;
    compile_errorf("[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        compile_errorf(" at end");
    }
    else if (token->type == TOKEN_ERROR) {
   // Nothing.
    }
    else {
        compile_errorf(" at '%.*s'", token->length, token->start);
    }

    compile_errorf(": %s\n", message);
    parser.had_error = true;
}

static void error(const char* message)
{
    error_at(&parser.previous, message);
}

static void error_at_current(const char* message)
{
    error_at(&parser.current, message);
}

static void advance()
{
    parser.previous = parser.current;

    for (;;) {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR)
            break;

        error_at_current(parser.current.start);
    }
}

static void consume(LoxTokenType type, const char* message)
{
    if (parser.current.type == type) {
        advance();
        return;
    }

    error_at_current(message);
}

static bool check(LoxTokenType type)
{
    return parser.current.type == type;
}

static void skip(LoxTokenType type)
{
    if (check(type))
        advance();
}

static bool match(LoxTokenType type)
{
    if (!check(type))
        return false;
    advance();
    return true;
}

static void emit_byte(uint8_t byte)
{
    write_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_word(uint16_t word)
{
    emit_bytes((uint8_t)(word >> 0) | 0x80, word & 0xFF);
}

static void emit_constant_index(int index)
{
    if (index < 0x80)
        emit_byte((uint8_t)index);
    else
        emit_word((uint16_t)index);
}

static void emit_loop(int loop_start)
{
    emit_byte(OP_LOOP);

    int offset = current_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");

    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
}

static int emit_jump(uint8_t instruction)
{
    emit_byte(instruction);
    emit_byte(0xff);
    emit_byte(0xff);
    return current_chunk()->count - 2;
}

static void emit_return()
{
    if (current->type == TYPE_INITIALIZER) {
        emit_bytes(OP_GET_LOCAL, 0);
    }
    else {
        emit_byte(OP_NIL);
    }
    emit_byte(OP_RETURN);
}

static int make_constant(Value value)
{
    int constant = add_constant(current_chunk(), value);
    if (constant > 0x7FFF) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return constant;
}

static ObjFunction* end_compiler()
{
    emit_return();
    ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error) {
        disassemble_chunk(current_chunk(), function->name != NULL
            ? function->name->chars : "<script>");
    }
#endif

    current = current->enclosing;
    return function;
}

static void begin_scope()
{
    current->scope_depth++;
}

static void end_scope()
{
    current->scope_depth--;

    while (current->local_count > 0 && 
        current->locals[current->local_count - 1].depth > 
            current->scope_depth) {
        Local* local = &current->locals[current->local_count - 1];
        if (local->occupies_slot) {
            if (local->is_captured) {
                emit_byte(OP_CLOSE_UPVALUE);
            }
            else {
                emit_byte(OP_POP);
            }
            if (current->slot_count > 0) {
                current->slot_count--;
            }
        }
        current->local_count--;
    }
}

static void block();
static void expression();
static void statement();
static void declaration();
static ParseRule* get_rule(LoxTokenType type);
static void parse_precedence(Precedence precedence);
static bool evaluate_constant_expression(Chunk* chunk, int start, int end, Value* out_value);
static bool resolve_const_binding(Compiler* compiler, Token* name, Value* out_value);
static void discard_expression();
static void enum_declaration();
static void define_const_value(Token* name, Value value);
static void add_global_const(Token* name, Value value);
static bool get_global_const(Token* name, Value* out_value);
static bool has_global_const(Token* name);
static bool value_to_enum_int(Value value, int32_t* out_value);
static Local* declare_const_local(Token* name);

static bool identifiers_equal(Token* a, Token* b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

int resolve_local(Compiler* compiler, Token* name)
{
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static int add_upvalue(Compiler* compiler, uint8_t index, bool is_local)
{
    int upvalue_count = compiler->function->upvalue_count;

    for (int i = 0; i < upvalue_count; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->is_local == is_local) {
            return i;
        }
    }

    if (upvalue_count == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalue_count].is_local = is_local;
    compiler->upvalues[upvalue_count].index = index;
    return compiler->function->upvalue_count++;
}

static int resolve_upvalue(Compiler* compiler, Token* name)
{
    if (compiler->enclosing == NULL) return -1;

    int local = resolve_local(compiler->enclosing, name);
    if (local != -1) {
        Local* local_entry = &compiler->enclosing->locals[local];
        if (local_entry->is_const || local_entry->slot < 0) {
            return -1;
        }
        compiler->enclosing->locals[local].is_captured = true;
        return add_upvalue(compiler, (uint8_t)local_entry->slot, true);
    }

    int upvalue = resolve_upvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return add_upvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

static bool resolve_const_binding(Compiler* compiler, Token* name, Value* out_value)
{
    for (Compiler* c = compiler; c != NULL; c = c->enclosing) {
        for (int i = c->local_count - 1; i >= 0; i--) {
            Local* local = &c->locals[i];
            if (!identifiers_equal(name, &local->name)) continue;

            if (!local->is_const) {
                return false;
            }

            if (!local->has_const_value) {
                return false;
            }

            *out_value = local->const_value;
            return true;
        }
    }

    if (get_global_const(name, out_value)) {
        return true;
    }

    return false;
}

static bool get_global_const(Token* name, Value* out_value)
{
    ObjString* name_str = copy_string(name->start, name->length);
    if (table_get(&global_const_table, name_str, out_value)) {
        return true;
    }

    return false;
}

static bool has_global_const(Token* name)
{
    Value value;
    return get_global_const(name, &value);
}

static bool value_to_enum_int(Value value, int32_t* out_value)
{
#ifdef NAN_BOXING
    if (IS_INT(value)) {
        *out_value = AS_INT(value);
        return true;
    }
    if (IS_DOUBLE(value)) {
        double d = AS_DOUBLE(value);
        if (!isfinite(d))
            return false;
        double truncated = d < 0 ? ceil(d) : floor(d);
        if (truncated != d)
            return false;
        if (truncated < (double)INT32_MIN || truncated > (double)INT32_MAX)
            return false;
        *out_value = (int32_t)truncated;
        return true;
    }
#else
    if (IS_NUMBER(value)) {
        double d = AS_NUMBER(value);
        if (!isfinite(d))
            return false;
        double truncated = d < 0 ? ceil(d) : floor(d);
        if (truncated != d)
            return false;
        if (truncated < (double)INT32_MIN || truncated > (double)INT32_MAX)
            return false;
        *out_value = (int32_t)truncated;
        return true;
    }
#endif
    return false;
}

static void add_global_const(Token* name, Value value)
{
    ObjString* name_str = copy_string(name->start, name->length);

    Value existing;
    if (table_get(&global_const_table, name_str, &existing) && !test_output_enabled) {
        error("Already a constant with this name.");
        return;
    }

    push(value);
    table_set(&global_const_table, name_str, value);
    pop();
}

static void define_const_value(Token* name, Value value)
{
    if (current->scope_depth > 0) {
        Local* local = declare_const_local(name);
        if (local != NULL) {
            local->const_value = value;
            local->has_const_value = true;
            local->depth = current->scope_depth;
        }
    }
    else {
        add_global_const(name, value);
    }
}

static Local* add_local(Token name, bool occupies_slot, bool is_const)
{
    if (current->local_count == UINT8_COUNT) {
        error("Too many local variables in function.");
        return NULL;
    }

    Local* local = &current->locals[current->local_count++];
    local->name = name;
    local->depth = -1;
    local->is_captured = false;
    local->is_const = is_const;
    local->occupies_slot = occupies_slot;
    local->has_const_value = false;
    local->const_value = NIL_VAL;
    if (occupies_slot) {
        local->slot = current->slot_count++;
    }
    else {
        local->slot = -1;
    }
    return local;
}

static void declare_variable()
{
    if (current->scope_depth == 0) return;

    Token* name = &parser.previous;
    for (int i = current->local_count - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scope_depth) {
            break;
        }

        if (identifiers_equal(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    add_local(*name, true, false);
}

static Local* declare_const_local(Token* name)
{
    for (int i = current->local_count - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scope_depth) {
            break;
        }

        if (identifiers_equal(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    return add_local(*name, false, true);
}

static int identifier_constant(Token* name)
{
    return make_constant(OBJ_VAL(copy_string(name->start, name->length)));
}

static int parse_variable(const char* error_message)
{
    consume(TOKEN_IDENTIFIER, error_message);

    if (current->scope_depth == 0 && has_global_const(&parser.previous)) {
        error("Cannot redeclare constant with this name.");
    }

    declare_variable();
    if (current->scope_depth > 0)
        return 0;

    return identifier_constant(&parser.previous);
}

static void mark_initialized()
{
    if (current->scope_depth == 0)
        return;
    current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void define_variable(int global)
{
    if (current->scope_depth > 0) {
        mark_initialized();
        return;
    }

    emit_byte(OP_DEFINE_GLOBAL);
    emit_constant_index(global);
}

static uint8_t argument_list()
{
    uint8_t arg_count = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (arg_count == 255) {
                error("Can't have more than 255 arguments.");
            }
            arg_count++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return arg_count;
}

static void patch_jump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    current_chunk()->code[offset] = (jump >> 8) & 0xff;
    current_chunk()->code[offset + 1] = jump & 0xff;
}

static void patch_loop_exits()
{
    if (current->current_loop != NULL) {
        for (int i = 0; i < current->current_loop->exit_patch_count; ++i) {
            int patch_offset = current->current_loop->exit_patches[i];
            patch_jump(patch_offset);
        }
    }
}

static void and_(bool can_assign)
{
    int endJump = emit_jump(OP_JUMP_IF_FALSE);

    emit_byte(OP_POP);
    parse_precedence(PREC_AND);

    patch_jump(endJump);
}

static void binary(bool can_assign)
{
    LoxTokenType operator_type = parser.previous.type;
    ParseRule* rule = get_rule(operator_type);
    parse_precedence((Precedence)(rule->precedence + 1));

    switch (operator_type) {
    case TOKEN_BANG_EQUAL:    emit_bytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
    case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emit_byte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emit_bytes(OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:          emit_byte(OP_ADD); break;
    case TOKEN_MINUS:         emit_byte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emit_byte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emit_byte(OP_DIVIDE); break;
    default: return; // Unreachable.
    }
}

static void call(bool can_assign)
{
    uint8_t arg_count = argument_list();
    emit_bytes(OP_CALL, arg_count);
}

static void each(bool assign)
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after '.each'");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after '.each' expression.");
    emit_byte(OP_EACH_PRIME);

    LoopContext each_loop = { 0 };
    each_loop.enclosing = current->current_loop;
    each_loop.scope_depth = current->scope_depth;
    current->current_loop = &each_loop;

    each_loop.loop_start = current_chunk()->count;
    each_loop.exit_jump = emit_jump(OP_EACH_OR_END);
    emit_byte(OP_POP); // Pop old iterated value added by OP_EACH_OR_END
    emit_byte(OP_EACH_ADVANCE);
    emit_loop(each_loop.loop_start);

    patch_jump(each_loop.exit_jump);
    patch_loop_exits();

    current->current_loop = current->current_loop->enclosing;
}

static void resolve_property(Token* token, bool can_assign)
{
    int name = identifier_constant(token);

    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        emit_byte(OP_SET_PROPERTY);
        emit_constant_index(name);
    }
    else if (match(TOKEN_LEFT_PAREN)) {
        uint8_t arg_count = argument_list();
        emit_byte(OP_INVOKE);
        emit_constant_index(name);
        emit_byte(arg_count);
    }
    else {
        emit_byte(OP_GET_PROPERTY);
        emit_constant_index(name);
    }
}

static void dot(bool can_assign)
{
    if (match(TOKEN_EACH)) {
        each(can_assign);
        return;
    }

    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    resolve_property(&parser.previous, can_assign);
}

static void arr_index(bool can_assign)
{
    expression();
    consume(TOKEN_RIGHT_BRACK, "Expect ']' after index.");

    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        emit_byte(OP_SET_AT_INDEX);
    }
    else {
        emit_byte(OP_GET_AT_INDEX);
    }
}

static void literal(bool can_assign)
{
    switch (parser.previous.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_NIL: emit_byte(OP_NIL); break;
    case TOKEN_TRUE: emit_byte(OP_TRUE); break;
    case TOKEN_SELF: emit_byte(OP_SELF); break;
    default: return; // Unreachable.
    }
}

static void init_compiler(Compiler* compiler, FunctionType type)
{
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->local_count = 0;
    compiler->slot_count = 0;
    compiler->scope_depth = 0;
    compiler->function = new_function();
    compiler->current_loop = NULL;
    current = compiler;
    if (type != TYPE_SCRIPT && type != TYPE_LAMDA) {
        current->function->name = copy_string(parser.previous.start,
            parser.previous.length);
    }

    Token placeholder = { 0 };
    placeholder.start = "";
    placeholder.length = 0;
    Local* local = add_local(placeholder, true, false);
    if (local != NULL) {
        local->depth = 0;
    }
    if (type != TYPE_FUNCTION && type != TYPE_LAMDA) {
        if (local != NULL) {
            local->name.start = "this";
            local->name.length = 4;
        }
    }
    else {
        if (local != NULL) {
            local->name.start = "";
            local->name.length = 0;
        }
    }
}

static void lamda(bool can_assign)
{
    Compiler compiler;
    init_compiler(&compiler, TYPE_LAMDA);
    begin_scope();

    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                error_at_current("Can't have more than 255 parameters.");
            }
            int constant = parse_variable("Expect parameter name.");
            define_variable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_ARROW, "Expect '->' in lamda declaration.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before lamda body.");
    block();

    ObjFunction* function = end_compiler();
    emit_byte(OP_CLOSURE);
    emit_constant_index(make_constant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalue_count; i++) {
        emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
        emit_byte(compiler.upvalues[i].index);
    }
}

static bool is_lamda()
{
    // We saw a L_PAREN + IDENT.
    // Is this a lamda?

    // Save the old state; we'll come back to it.
    // FYI I hate doing this; but since this is a single-pass LR(1) compiler and
    // the grammar for lamda's requires LR(>2), it is a necessary evil.
    Scanner current_scanner = scanner;

#define NOT_LAMDA() \
    { scanner = current_scanner; return false; }

    if (parser.current.type == TOKEN_IDENTIFIER) {
        Token tok = scan_token();
        while (tok.type == TOKEN_COMMA) {
            tok = scan_token();
            if (tok.type != TOKEN_IDENTIFIER)
                NOT_LAMDA()
                tok = scan_token();
        }

        if (tok.type != TOKEN_RIGHT_PAREN)
            NOT_LAMDA()
    }

    if (scan_token().type != TOKEN_ARROW)
        NOT_LAMDA()

    // Now we have seen
    //      ( IDENT [, IDENT]* ) ->
    // It is safe so assume this is a lamda, because it's not valid for anything else.

#undef NOT_LAMDA

    scanner = current_scanner;
    return true;
}

static void grouping(bool can_assign)
{
    if ((parser.current.type == TOKEN_IDENTIFIER 
        || parser.current.type == TOKEN_RIGHT_PAREN) && is_lamda()) {
        lamda(can_assign);
    }
    else {
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    }
}

static void emit_constant(Value value)
{
    emit_byte(OP_CONSTANT);
    emit_constant_index(make_constant(value));
}

#define CONST_EVAL_STACK_MAX 256

static bool const_is_number(Value value)
{
    return IS_INT(value) || IS_DOUBLE(value);
}

static double const_to_double(Value value)
{
    return IS_DOUBLE(value) ? AS_DOUBLE(value) : (double)AS_INT(value);
}

static bool const_is_falsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static int read_constant_index_for_eval(Chunk* chunk, int* ip, int end)
{
    if (*ip >= end) return -1;
    int constant = chunk->code[(*ip)++];
    if (!(constant & 0x80)) return constant;
    if (*ip >= end) return -1;
    int result = (constant & 0x7f) << 8;
    result |= chunk->code[(*ip)++];
    return result;
}

static bool evaluate_constant_expression(Chunk* chunk, int start, int end, Value* out_value)
{
    Value stack[CONST_EVAL_STACK_MAX];
    int top = 0;
    int ip = start;

    while (ip < end) {
        uint8_t instruction = chunk->code[ip++];
        switch (instruction) {
        case OP_CONSTANT: {
                int constant_index = read_constant_index_for_eval(chunk, &ip, end);
                if (constant_index < 0 || constant_index >= chunk->constants.count) return false;
                if (top >= CONST_EVAL_STACK_MAX) return false;
                stack[top++] = chunk->constants.values[constant_index];
                break;
            }
        case OP_NIL:
            if (top >= CONST_EVAL_STACK_MAX) return false;
            stack[top++] = NIL_VAL;
            break;
        case OP_TRUE:
            if (top >= CONST_EVAL_STACK_MAX) return false;
            stack[top++] = BOOL_VAL(true);
            break;
        case OP_FALSE:
            if (top >= CONST_EVAL_STACK_MAX) return false;
            stack[top++] = BOOL_VAL(false);
            break;
        case OP_ADD:
        case OP_SUBTRACT:
        case OP_MULTIPLY:
        case OP_DIVIDE: {
                if (top < 2) return false;
                Value b = stack[--top];
                Value a = stack[--top];
                if (!const_is_number(a) || !const_is_number(b)) return false;
                if (IS_DOUBLE(a) || IS_DOUBLE(b)) {
                    double da = const_to_double(a);
                    double db = const_to_double(b);
                    double result;
                    switch (instruction) {
                    case OP_ADD: result = da + db; break;
                    case OP_SUBTRACT: result = da - db; break;
                    case OP_MULTIPLY: result = da * db; break;
                    default: result = da / db; break;
                    }
                    if (top >= CONST_EVAL_STACK_MAX) return false;
                    stack[top++] = DOUBLE_VAL(result);
                }
                else {
                    int32_t ia = AS_INT(a);
                    int32_t ib = AS_INT(b);
                    if (instruction == OP_DIVIDE && ib == 0) return false;
                    int32_t result;
                    switch (instruction) {
                    case OP_ADD: result = ia + ib; break;
                    case OP_SUBTRACT: result = ia - ib; break;
                    case OP_MULTIPLY: result = ia * ib; break;
                    default: result = ia / ib; break;
                    }
                    if (top >= CONST_EVAL_STACK_MAX) return false;
                    stack[top++] = INT_VAL(result);
                }
                break;
            }
        case OP_NOT: {
                if (top < 1) return false;
                Value value = stack[--top];
                if (top >= CONST_EVAL_STACK_MAX) return false;
                stack[top++] = BOOL_VAL(const_is_falsey(value));
                break;
            }
        case OP_NEGATE: {
                if (top < 1) return false;
                Value value = stack[--top];
                if (IS_DOUBLE(value)) {
                    stack[top++] = DOUBLE_VAL(-AS_DOUBLE(value));
                }
                else if (IS_INT(value)) {
                    stack[top++] = INT_VAL(-AS_INT(value));
                }
                else {
                    return false;
                }
                break;
            }
        case OP_EQUAL: {
                if (top < 2) return false;
                Value b = stack[--top];
                Value a = stack[--top];
                if (top >= CONST_EVAL_STACK_MAX) return false;
                stack[top++] = BOOL_VAL(values_equal(a, b));
                break;
            }
        case OP_GREATER:
        case OP_LESS: {
                if (top < 2) return false;
                Value b = stack[--top];
                Value a = stack[--top];
                if (!const_is_number(a) || !const_is_number(b)) return false;
                bool result;
                if (IS_DOUBLE(a) || IS_DOUBLE(b)) {
                    double da = const_to_double(a);
                    double db = const_to_double(b);
                    result = (instruction == OP_GREATER) ? (da > db) : (da < db);
                }
                else {
                    int32_t ia = AS_INT(a);
                    int32_t ib = AS_INT(b);
                    result = (instruction == OP_GREATER) ? (ia > ib) : (ia < ib);
                }
                if (top >= CONST_EVAL_STACK_MAX) return false;
                stack[top++] = BOOL_VAL(result);
                break;
            }
        default:
            return false;
        }
    }

    if (top != 1) return false;
    *out_value = stack[0];
    return true;
}

static void number(bool can_assign)
{
    if (parser.previous.type == TOKEN_DOUBLE) {
        double value = strtod(parser.previous.start, NULL);
        emit_constant(DOUBLE_VAL(value));
    }
    else {
        int32_t value = strtol(parser.previous.start, NULL, 10);
        emit_constant(INT_VAL(value));
    }
}

static void or_(bool can_assign)
{
    int else_jump = emit_jump(OP_JUMP_IF_FALSE);
    int end_jump = emit_jump(OP_JUMP);

    patch_jump(else_jump);
    emit_byte(OP_POP);

    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

static Token synthetic_token(const char* text)
{
    Token token = { 0 };
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

static void emit_read_variable(uint8_t op, int arg)
{
    emit_byte(op);
    if (op == OP_GET_GLOBAL) {
        emit_constant_index(arg);
    }
    else {
        emit_byte((uint8_t)arg);
    }
}

static void emit_write_variable(uint8_t op, int arg)
{
    emit_byte(op);
    if (op == OP_SET_GLOBAL) {
        emit_constant_index(arg);
    }
    else {
        emit_byte((uint8_t)arg);
    }
}

static void discard_expression()
{
    Chunk* chunk = current_chunk();
    int start = chunk->count;
    int constant_mark = chunk->constants.count;

    expression();

    chunk->count = start;
    chunk->constants.count = constant_mark;
}

static void named_variable(Token name, bool can_assign)
{
    Value const_value;
    if (resolve_const_binding(current, &name, &const_value)) {
        if (can_assign) {
            if (check(TOKEN_EQUAL) || check(TOKEN_PLUS_PLUS) ||
                check(TOKEN_MINUS_MINUS) || check(TOKEN_PLUS_EQUALS) ||
                check(TOKEN_MINUS_EQUALS)) {
                error("Cannot assign to const variable.");
            }
            if (match(TOKEN_EQUAL)) {
                discard_expression();
                return;
            }
            if (match(TOKEN_PLUS_PLUS) || match(TOKEN_MINUS_MINUS)) {
                return;
            }
            if (match(TOKEN_PLUS_EQUALS) || match(TOKEN_MINUS_EQUALS)) {
                discard_expression();
                return;
            }
        }

        if (IS_ENUM(const_value)) {
            consume(TOKEN_DOT, "Expected '.' after enum.");
            consume(TOKEN_IDENTIFIER, "Expected enum specifier.");

            ObjEnum* enum_obj = AS_ENUM(const_value);
            Value enum_val;

            ObjString* enum_member = copy_string(parser.previous.start, parser.previous.length);
            if (!table_get(&enum_obj->values, enum_member, &enum_val)) {
                error_at(&parser.previous, "Enum member not found.");
                enum_val = NIL_VAL;
            }

            emit_constant(enum_val);
            return;
        }

        emit_constant(const_value);
        return;
    }

    uint8_t get_op, set_op;
    int arg;
    int local_index = resolve_local(current, &name);
    if (local_index != -1) {
        Local* local = &current->locals[local_index];
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
        arg = local->slot;
    }
    else {
        int upvalue = resolve_upvalue(current, &name);
        if (upvalue != -1) {
            get_op = OP_GET_UPVALUE;
            set_op = OP_SET_UPVALUE;
            arg = upvalue;
        }
        else {
            arg = identifier_constant(&name);
            get_op = OP_GET_GLOBAL;
            set_op = OP_SET_GLOBAL;
        }
    }

    if (get_op == OP_GET_LOCAL || get_op == OP_SET_LOCAL) {
        if (arg < 0) {
            error("Invalid local slot for variable.");
            arg = 0;
        }
    }

    if (can_assign) {
        if (match(TOKEN_EQUAL)) {
            expression();
            emit_write_variable(set_op, arg);
        }
        else if (match(TOKEN_PLUS_PLUS)) {
            emit_read_variable(get_op, arg); // Post-fix; preserve original
            emit_read_variable(get_op, arg);
            emit_constant(INT_VAL(1));
            emit_byte(OP_ADD);
            emit_write_variable(set_op, arg);
            emit_byte(OP_POP);
        }
        else if (match(TOKEN_MINUS_MINUS)) {
            emit_read_variable(get_op, arg); // Post-fix; preserve original
            emit_read_variable(get_op, arg);
            emit_constant(INT_VAL(1));
            emit_byte(OP_SUBTRACT);
            emit_write_variable(set_op, arg);
            emit_byte(OP_POP);
        }
        else if (match(TOKEN_PLUS_EQUALS)) {
            emit_read_variable(get_op, arg);
            expression();
            emit_byte(OP_ADD);
            emit_write_variable(set_op, arg);
        }
        else if (match(TOKEN_MINUS_EQUALS)) {
            emit_read_variable(get_op, arg);
            expression();
            emit_byte(OP_SUBTRACT);
            emit_write_variable(set_op, arg);
        }
        else {
            emit_read_variable(get_op, arg);
        }
    }
    else {
        emit_read_variable(get_op, arg);
    }
}

//static void string(bool can_assign)
//{
//    emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1,
//        parser.previous.length - 2)));
//}

static void variable(bool can_assign)
{
    Token token = parser.previous;
    
    if (current_class != NULL && current_class->entity_this 
        && strcmp(token.start, "this")) {
        // Entities that have classes compiled against them have some native
        // "methods" that I don't want to speculatively cram into their 
        // fields.
        ObjString* name_str = copy_string(token.start, token.length);
        push(OBJ_VAL(name_str));
        Value value;
        if (table_get(&native_methods, name_str, &value)
            && IS_NATIVE_METHOD(value)) {
            pop();
            // Add a fake "this" to get the entity on the stack.
            named_variable(synthetic_token("this"), can_assign);
            // Resolve as if it were a property.
            resolve_property(&token, can_assign);
            return;
        }
        else
            pop();
    }

    named_variable(parser.previous, can_assign);
}

static void string_interp(bool can_assign)
{
    int count = 0;

    Token first = parser.previous;

    while (parser.previous.type == TOKEN_STRING_INTERP || parser.previous.type == TOKEN_STRING) {
        if (parser.previous.type == TOKEN_STRING_INTERP) {
            // Skip the first character (a `"` or `{`); but keep the last.
            emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1,
                parser.previous.length - 1)));
            count++;

            consume(TOKEN_DOLLAR, "Expected '$' interpolation character");
            consume(TOKEN_LEFT_BRACE, "Expected '{' interpolation delimiter");

            expression();

            consume(TOKEN_RIGHT_BRACE, "Expected '}' interpolation delimiter");
            count++;

            if (!match(TOKEN_STRING) && !match(TOKEN_STRING_INTERP)) {
                error_at(&first, "Malformed string interpolation");
                return;
            }
        }
        else {
            // Skip the first character (`"` or `}` if following an string
            // interpolation) and the last character (`"`).
            emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1,
                parser.previous.length - 2)));
            count++;

            // If we have stacked strings, we want to concatenate them. We prime
            // pump for the next iteration by moving the next string into 
            // parser.previous. But if it's not a string, we want to leave it as
            // parser.current for the next parse routine.
            if (check(TOKEN_STRING) || check(TOKEN_STRING_INTERP))
                advance();
            else
                break;
        }
    }

    if (count > UINT8_MAX) {
        error_at(&first, "String interpolation exceeded maximum count.");
        count = UINT8_MAX;
    }
    
    if (count > 1)
        emit_bytes(OP_INTERP, (uint8_t)count);
}

static uint8_t element_list()
{
    uint8_t elem_count = 0;
    if (!check(TOKEN_RIGHT_BRACK)) {
        do {
            expression();
            if (elem_count == 255) {
                error("Can't have more than 255 elements.");
            }
            elem_count++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_BRACK, "Expect ']' after elements.");
    return elem_count;
}

static void array_(bool can_assign)
{
    uint8_t elem_count = element_list();
    emit_bytes(OP_ARRAY, elem_count);
}

static void super_(bool can_assign)
{
    if (current_class == NULL) {
        error("Can't use 'super' outside of a class.");
    }
    else if (!current_class->has_superclass) {
        error("Can't use 'super' in a class with no superclass.");
    }

    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    int name = identifier_constant(&parser.previous);

    named_variable(synthetic_token("this"), false);
    if (match(TOKEN_LEFT_PAREN)) {
        uint8_t arg_count = argument_list();
        named_variable(synthetic_token("super"), false);
        emit_byte(OP_SUPER_INVOKE);
        emit_constant_index(name);
        emit_byte(arg_count);
    }
    else {
        named_variable(synthetic_token("super"), false);
        emit_byte(OP_GET_SUPER);
        emit_constant_index(name);
    }
}

static void this_(bool can_assign)
{
    if (current_class == NULL) {
        error("Can't use 'this' outside of a class.");
        return;
    }

    variable(false);
}

static void unary(bool can_assign)
{
    LoxTokenType operator_type = parser.previous.type;

    // Compile the operand.
    parse_precedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operator_type) {
    case TOKEN_BANG: emit_byte(OP_NOT); break;
    case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
    default: return; // Unreachable.
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]      = { grouping,       call,       PREC_CALL       },
    [TOKEN_RIGHT_PAREN]     = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_LEFT_BRACE]      = { NULL,           NULL,       PREC_NONE       }, 
    [TOKEN_RIGHT_BRACE]     = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_LEFT_BRACK]      = { array_,         arr_index,  PREC_CALL       }, 
    [TOKEN_RIGHT_BRACK]     = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_COMMA]           = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_DOT]             = { NULL,           dot,        PREC_CALL       },
    [TOKEN_MINUS]           = { unary,          binary,     PREC_TERM       },
    [TOKEN_PLUS]            = { NULL,           binary,     PREC_TERM       },
    [TOKEN_SEMICOLON]       = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_COLON]           = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_SLASH]           = { NULL,           binary,     PREC_FACTOR     },
    [TOKEN_STAR]            = { NULL,           binary,     PREC_FACTOR     },
    [TOKEN_DOLLAR]          = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_BANG]            = { unary,          NULL,       PREC_NONE       },
    [TOKEN_BANG_EQUAL]      = { NULL,           binary,     PREC_EQUALITY   },
    [TOKEN_EQUAL]           = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_EQUAL_EQUAL]     = { NULL,           binary,     PREC_EQUALITY   },
    [TOKEN_GREATER]         = { NULL,           binary,     PREC_COMPARISON },
    [TOKEN_GREATER_EQUAL]   = { NULL,           binary,     PREC_COMPARISON },
    [TOKEN_LESS]            = { NULL,           binary,     PREC_COMPARISON },
    [TOKEN_LESS_EQUAL]      = { NULL,           binary,     PREC_COMPARISON },
    [TOKEN_PLUS_PLUS]       = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_MINUS_MINUS]     = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_PLUS_EQUALS]     = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_MINUS_EQUALS]    = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_ARROW]           = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_IDENTIFIER]      = { variable,       NULL,       PREC_NONE       },
    [TOKEN_STRING]          = { string_interp,  NULL,       PREC_NONE       },
    [TOKEN_STRING_INTERP]   = { string_interp,  NULL,       PREC_NONE       },
    [TOKEN_INT]             = { number,         NULL,       PREC_NONE       },
    [TOKEN_DOUBLE]          = { number,         NULL,       PREC_NONE       },
    [TOKEN_AND]             = { NULL,           and_,       PREC_AND        },
    [TOKEN_CLASS]           = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_CONST]           = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_ELSE]            = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_FALSE]           = { literal,        NULL,       PREC_NONE       },
    [TOKEN_FOR]             = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_FUN]             = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_IF]              = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_NIL]             = { literal,        NULL,       PREC_NONE       },
    [TOKEN_OR]              = { NULL,           or_,        PREC_OR         },
    [TOKEN_PRINT]           = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_RETURN]          = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_SUPER]           = { super_,         NULL,       PREC_NONE       },
    [TOKEN_THIS]            = { this_,          NULL,       PREC_NONE       },
    [TOKEN_TRUE]            = { literal,        NULL,       PREC_NONE       },
    [TOKEN_VAR]             = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_WHILE]           = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_EACH]            = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_BREAK]           = { NULL,           NULL,       PREC_NONE       }, /* not used yet */
    [TOKEN_CONTINUE]        = { NULL,           NULL,       PREC_NONE       }, /* not used yet */
    [TOKEN_SELF]            = { literal,        NULL,       PREC_NONE       },
    [TOKEN_ERROR]           = { NULL,           NULL,       PREC_NONE       },
    [TOKEN_EOF]             = { NULL,           NULL,       PREC_NONE       },
};

static void parse_precedence(Precedence precedence)
{
    advance();
    ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL) {
        error("Expect expression.");
        return;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(can_assign);

    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule(can_assign);
    }

    if (can_assign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static ParseRule* get_rule(LoxTokenType type)
{
    return &rules[type];
}

static void expression()
{
    parse_precedence(PREC_ASSIGNMENT);
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type)
{
    Compiler compiler;
    init_compiler(&compiler, type);
    begin_scope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                error_at_current("Can't have more than 255 parameters.");
            }
            int constant = parse_variable("Expect parameter name.");
            define_variable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction* function = end_compiler();
    emit_byte(OP_CLOSURE);
    emit_constant_index(make_constant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalue_count; i++) {
        emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
        emit_byte(compiler.upvalues[i].index);
    }
}

static void method()
{
    consume(TOKEN_IDENTIFIER, "Expect method name.");
    int constant = identifier_constant(&parser.previous);

    FunctionType type = TYPE_METHOD;
    if (parser.previous.length == 4 &&
        memcmp(parser.previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }

    function(type);

    emit_byte(OP_METHOD);
    emit_constant_index(constant);
}

static void class_declaration()
{
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    Token class_name = parser.previous;
    int name_constant = identifier_constant(&parser.previous);
    declare_variable();

    emit_byte(OP_CLASS);
    emit_constant_index(name_constant);
    define_variable(name_constant);

    ClassCompiler class_compiler = { 0 };
    class_compiler.has_superclass = false;
    class_compiler.enclosing = current_class;
    class_compiler.entity_this = NULL;
    current_class = &class_compiler;

    if (compile_context.this_ != NULL) {
        current_class->entity_this = compile_context.this_;
        compile_context.this_ = NULL;   // Only for this class scope.
    }

    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENTIFIER, "Expect superclass name.");
        variable(false);

        if (identifiers_equal(&class_name, &parser.previous)) {
            error("A class can't inherit from itself.");
        }

        begin_scope();
        add_local(synthetic_token("super"), true, false);
        define_variable(0);

        named_variable(class_name, false);
        emit_byte(OP_INHERIT);
        class_compiler.has_superclass = true;
    }

    named_variable(class_name, false);
    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        method();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    emit_byte(OP_POP);

    if (class_compiler.has_superclass) {
        end_scope();
    }

    current_class = current_class->enclosing;
}

static void fun_declaration()
{
    int global = parse_variable("Expect function name.");
    mark_initialized();
    function(TYPE_FUNCTION);
    define_variable(global);
}

static void enum_declaration()
{
    consume(TOKEN_IDENTIFIER, "Expect enum name.");
    Token enum_name = parser.previous;
    ObjString* enum_name_str = copy_string(enum_name.start, enum_name.length);

    ObjEnum* enum_obj = new_enum(enum_name_str);
    push(OBJ_VAL(enum_obj));

    consume(TOKEN_LEFT_BRACE, "Expect '{' after enum name.");

    int32_t next_value = 0;

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        consume(TOKEN_IDENTIFIER, "Expect enum member name.");
        Token member_name = parser.previous;

        int start = current_chunk()->count;
        int constant_mark = current_chunk()->constants.count;

        bool has_initializer = match(TOKEN_EQUAL);
        int32_t member_int = next_value;
        if (has_initializer) {
            expression();
        }

        int end = current_chunk()->count;

        if (has_initializer) {
            Value evaluated;
            bool ok = evaluate_constant_expression(current_chunk(), start, end, &evaluated);
            if (!ok || !value_to_enum_int(evaluated, &member_int)) {
                error("Enum value must be an integer constant expression.");
                member_int = next_value;
            }
        }

        current_chunk()->count = start;
        current_chunk()->constants.count = constant_mark;

        if (member_int == INT32_MAX) {
            next_value = INT32_MAX;
        }
        else {
            next_value = member_int + 1;
        }

        Value member_value = INT_VAL(member_int);

        ObjString* member_name_str = copy_string(member_name.start, member_name.length);
        Value existing;
        bool inserted = true;
        if (table_get(&enum_obj->values, member_name_str, &existing)) {
            error("Duplicate enum member name.");
            inserted = false;
        }
        else {
            table_set(&enum_obj->values, member_name_str, member_value);
        }

        if (inserted) {
            define_const_value(&member_name, member_value);
        }

        if (!match(TOKEN_COMMA)) {
            break;
        }
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after enum body.");

    define_const_value(&enum_name, OBJ_VAL(enum_obj));
    pop();

    skip(TOKEN_SEMICOLON);
}

static void var_declaration()
{
    int global = parse_variable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    }
    else {
        emit_byte(OP_NIL);
    }

    skip(TOKEN_SEMICOLON);

    define_variable(global);
}

static void const_declaration()
{
    consume(TOKEN_IDENTIFIER, "Expect constant name.");
    Token name = parser.previous;

    Local* local = NULL;
    bool is_local = current->scope_depth > 0;
    if (is_local) {
        local = declare_const_local(&name);
    }
    else if (has_global_const(&name) && !test_output_enabled) {
        error("Already a constant with this name.");
    }

    consume(TOKEN_EQUAL, "Expect '=' after constant name.");

    Chunk* chunk = current_chunk();
    int start = chunk->count;
    int constant_mark = chunk->constants.count;

    expression();

    int end = chunk->count;

    Value const_value;
    bool ok = evaluate_constant_expression(chunk, start, end, &const_value);

    chunk->count = start;
    chunk->constants.count = constant_mark;

    skip(TOKEN_SEMICOLON);

    if (!ok) {
        error("Const initializer must be a compile-time constant expression.");
        return;
    }

    if (is_local) {
        if (local != NULL) {
            local->const_value = const_value;
            local->has_const_value = true;
            local->depth = current->scope_depth;
        }
    }
    else {
        add_global_const(&name, const_value);
    }

}

static void expression_statement()
{
    expression();

    skip(TOKEN_SEMICOLON);

    if (exec_context.is_repl && current->function->name == NULL) {
        emit_byte(OP_PRINT);
    }
    else {
        emit_byte(OP_POP);
    }
}

static void for_statement()
{
    begin_scope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
        // No initializer.
    }
    else if (match(TOKEN_VAR)) {
        var_declaration();
    }
    else {
        expression_statement();
    }

    LoopContext for_loop = { 0 };
    for_loop.enclosing = current->current_loop;
    for_loop.scope_depth = current->scope_depth;
    current->current_loop = &for_loop;

    for_loop.loop_start = current_chunk()->count;
    for_loop.exit_jump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        for_loop.exit_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP); // Condition.
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        int body_jump = emit_jump(OP_JUMP);
        int increment_start = current_chunk()->count;
        expression();
        emit_byte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emit_loop(for_loop.loop_start);
        for_loop.loop_start = increment_start;
        patch_jump(body_jump);
    }

    statement();
    emit_loop(for_loop.loop_start);

    if (for_loop.exit_jump != -1) {
        patch_jump(for_loop.exit_jump);
        emit_byte(OP_POP); // Condition.
    }
    patch_loop_exits();

    current->current_loop = current->current_loop->enclosing;
    end_scope();
}

static void break_statement()
{
    skip(TOKEN_SEMICOLON);

    if (current->current_loop == NULL) {
        error("Must be in a loop to 'break'.");
        return;
    }

    if (current->current_loop->exit_patch_count == UINT8_COUNT) {
        error("Too many 'break' statements in loop.");
        return;
    }

    current->current_loop->exit_patches[current->current_loop->exit_patch_count++] =
        emit_jump(OP_JUMP);
}

static void continue_statement()
{
    skip(TOKEN_SEMICOLON);

    if (current->current_loop == NULL) {
        error("Must be in a loop to 'continue'.");
        return;
    }

    emit_loop(current->current_loop->loop_start);
}

static void if_statement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();

    int elseJump = emit_jump(OP_JUMP);

    patch_jump(then_jump);
    emit_byte(OP_POP);

    if (match(TOKEN_ELSE)) statement();
    patch_jump(elseJump);
}

static void print_statement()
{
    expression();

    skip(TOKEN_SEMICOLON);
    emit_byte(OP_PRINT);
}

static void return_statement()
{
    if (match(TOKEN_SEMICOLON)) {
        emit_return();
    }
    else {
        if (current->type == TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.");
        }
        expression();

        skip(TOKEN_SEMICOLON);
        emit_byte(OP_RETURN);
    }
}

static void while_statement()
{
    LoopContext while_loop = { 0 };
    while_loop.enclosing = current->current_loop;
    while_loop.scope_depth = current->scope_depth;
    current->current_loop = &while_loop;

    while_loop.loop_start = current_chunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    while_loop.exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();
    emit_loop(while_loop.loop_start);

    patch_jump(while_loop.exit_jump);
    emit_byte(OP_POP);

    patch_loop_exits();

    current->current_loop = current->current_loop->enclosing;
}

static void synchronize()
{
    parser.panic_mode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_ENUM:
        case TOKEN_CONST:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;

        default:
            ; // Do nothing.
        }

        advance();
    }
}

static void declaration()
{
    if (match(TOKEN_CLASS)) {
        class_declaration();
    } 
    else if (match(TOKEN_FUN)) {
        fun_declaration();
    }
    else if (match(TOKEN_ENUM)) {
        enum_declaration();
    }
    else if (match(TOKEN_CONST)) {
        const_declaration();
    }
    else if (match(TOKEN_VAR)) {
        var_declaration();
    }
    else {
        statement();
    }

    if (parser.panic_mode)
        synchronize();
}

static void statement()
{
    if (match(TOKEN_PRINT)) {
        print_statement();
    }
    else if (match(TOKEN_BREAK)) {
        break_statement();
    }
    else if (match(TOKEN_CONTINUE)) {
        continue_statement();
    }
    else if (match(TOKEN_IF)) {
        if_statement();
    }
    else if (match(TOKEN_RETURN)) {
        return_statement();
    }
    else if (match(TOKEN_WHILE)) {
        while_statement();
    }
    else if (match(TOKEN_FOR)) {
        for_statement();
    }
    else if (match(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    }
    else if (match(TOKEN_SEMICOLON)) {
        // Ignore extraneous semicolons
    }
    else {
        expression_statement();
    }
}

ObjFunction* compile(const char* source)
{
    init_scanner(source);
    Compiler compiler;
    init_compiler(&compiler, TYPE_SCRIPT);

    parser.had_error = false;
    parser.panic_mode = false;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = end_compiler();
    return parser.had_error ? NULL : function;
}

void mark_compiler_roots()
{
    Compiler* compiler = current;
    while (compiler != NULL) {
        mark_object((Obj*)compiler->function);
        for (int i = 0; i < compiler->local_count; i++) {
            Local* local = &compiler->locals[i];
            if (local->has_const_value && IS_OBJ(local->const_value)) {
                mark_value(local->const_value);
            }
        }
        compiler = compiler->enclosing;
    }
}
