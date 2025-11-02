////////////////////////////////////////////////////////////////////////////////
// scanner.c
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "scanner.h"

#include <stdio.h>
#include <string.h>

Scanner scanner;

void init_scanner(const char* source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.interp = STR_INT_NONE;
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_at_end()
{
    return *scanner.current == '\0';
}

static char advance()
{
    scanner.current++;
    return 
        scanner.current[-1];
}

static char peek()
{
    return *scanner.current;
}

static char peek_next()
{
    if (is_at_end()) 
        return '\0';

    return scanner.current[1];
}

static bool match(char expected)
{
    if (is_at_end())
        return false;
    
    if (*scanner.current != expected)
        return false;

    scanner.current++;
    return true;
}

static Token make_token(LoxTokenType type)
{
    Token token = { 0 };
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token error_token(const char* message)
{
    Token token = { 0 };
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

static void skip_whitespace()
{
    for (;;) {
        char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break;
        case '/':
            if (peek_next() == '/') {
              // A comment goes until the end of the line.
                while (peek() != '\n' && !is_at_end())
                    advance();
            }
            else {
                return;
            }
            break;
        default:
            return;
        }
    }
}

static LoxTokenType check_keyword(int start, int length, const char* rest, LoxTokenType type)
{
    if ((ptrdiff_t)(scanner.current - scanner.start) == (ptrdiff_t)start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static LoxTokenType identifier_type()
{
    switch (scanner.start[0]) {
    case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
    case 'b': return check_keyword(1, 4, "reak", TOKEN_BREAK);
    case 'c':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'l': return check_keyword(2, 3, "ass", TOKEN_CLASS);
            case 'o': {
                LoxTokenType type = check_keyword(1, 4, "onst", TOKEN_CONST);
                if (type != TOKEN_IDENTIFIER)
                    return type;
                return check_keyword(2, 6, "ntinue", TOKEN_CONTINUE);
            }
            }
        }
        break;
    case 'e': 
        if (scanner.current - scanner.start > 1) {
            switch(scanner.start[1]) {
            case 'a': return check_keyword(2, 2, "ch", TOKEN_EACH);
            case 'l': return check_keyword(2, 2, "se", TOKEN_ELSE);
            case 'n': return check_keyword(2, 2, "um", TOKEN_ENUM);
            }
        }
        break;
    case 'f':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
            case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
            case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
            }
        }
        break;
    case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
    case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
    case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
    case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': 
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'e': return check_keyword(2, 2, "lf", TOKEN_SELF);
            case 'u': return check_keyword(2, 3, "per", TOKEN_SUPER);
            }
        }
        break;
    case 't':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
            case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
        break;
    case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier()
{
    while (is_alpha(peek()) || is_digit(peek()))
        advance();

    return make_token(identifier_type());
}

static Token string()
{
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n')
            scanner.line++;
        else if (peek() == '$' && scanner.interp == STR_INT_NONE &&
                (peek_next() == '{')) {
            scanner.interp = STR_INT_START;
            return make_token(TOKEN_STRING_INTERP);
        }
        advance();
    }

    if (is_at_end())
        return error_token("Unterminated string.");

    // The closing quote.
    advance();
    return make_token(TOKEN_STRING);
}

static Token number()
{
    while (is_digit(peek()))
        advance();

    // Look for a fractional part.
    if (peek() == '.' && is_digit(peek_next())) {
        // Consume the ".".
        advance();

        while (is_digit(peek()))
            advance();
        return make_token(TOKEN_DOUBLE);
    }

    return make_token(TOKEN_INT);
}

Token scan_token()
{
    if (scanner.interp == STR_INT_END) {
        Token token = string();
        scanner.interp = STR_INT_NONE;
        return token;
    }

    skip_whitespace();

    scanner.start = scanner.current;

    if (is_at_end()) 
        return make_token(TOKEN_EOF);

    char c = advance();
    if (is_alpha(c))
        return identifier();
    if (is_digit(c)) 
        return number();

    switch (c) {
    case '(': return make_token(TOKEN_LEFT_PAREN);
    case ')': return make_token(TOKEN_RIGHT_PAREN);
    case '{': return make_token(TOKEN_LEFT_BRACE);
    case '}':  {
        if (scanner.interp == STR_INT_EXPR)
            scanner.interp = STR_INT_END;
        return make_token(TOKEN_RIGHT_BRACE);
    }
    case '[': return make_token(TOKEN_LEFT_BRACK);
    case ']': return make_token(TOKEN_RIGHT_BRACK);
    case ';': return make_token(TOKEN_SEMICOLON);
    case ':': return make_token(TOKEN_COLON);
    case ',': return make_token(TOKEN_COMMA);
    case '.': return make_token(TOKEN_DOT);
    case '-': return make_token(match('-') ? TOKEN_MINUS_MINUS :
        match('=') ? TOKEN_MINUS_EQUALS :
        match('>') ? TOKEN_ARROW : TOKEN_MINUS);
    case '+': return make_token(match('+') ? TOKEN_PLUS_PLUS : 
        match('=') ? TOKEN_PLUS_EQUALS : TOKEN_PLUS);
    case '/': return make_token(TOKEN_SLASH);
    case '*': return make_token(TOKEN_STAR);
    case '$': {
        if (scanner.interp == STR_INT_START) {
            if (peek() == '{')
                scanner.interp = STR_INT_EXPR;
            else
                return error_token("Unexpected string interpolation token.");
        }
        return make_token(TOKEN_DOLLAR);
    }
    case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return string();
    }

    return error_token("Unexpected character.");
}
