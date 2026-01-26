////////////////////////////////////////////////////////////////////////////////
// fileutils.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__FILEUTILS_H
#define MUD98__FILEUTILS_H

#include <stdbool.h>
#include <stdio.h>

void change_dir(const char* dir);
void close_file(FILE* fp);
bool file_exists(const char* dir);
FILE* open_append_file(const char* filename);
FILE* open_read_file(const char* filename);
FILE* open_write_file(const char* filename);
void close_reserve_file(void);
void open_reserve_file(void);

#define OPEN_OR(open_expr, or_expr)                                            \
    if ((open_expr) == NULL) {                                                 \
        or_expr;                                                               \
    }

#define OPEN_OR_DIE(open_expr)          OPEN_OR(open_expr, exit(1))
#define OPEN_OR_RETURN(open_expr)       OPEN_OR(open_expr, return)
#define OPEN_OR_RETURN_FALSE(open_expr) OPEN_OR(open_expr, return false)
#define OPEN_OR_RETURN_NULL(open_expr)  OPEN_OR(open_expr, return NULL)
#define OPEN_OR_BREAK(open_expr)        OPEN_OR(open_expr, break)
#define OPEN_OR_CONTINUE(open_expr)     OPEN_OR(open_expr, continue)

#endif // !MUD98__FILEUTILS_H
