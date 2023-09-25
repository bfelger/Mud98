////////////////////////////////////////////////////////////////////////////////
// file.c
////////////////////////////////////////////////////////////////////////////////

#include "file.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER
#define NULL_FILE   "/dev/null"     // To reserve one stream
#else
#include <direct.h>
#include <errno.h>
#include <io.h>
#define NULL_FILE   "nul"
#define F_OK 0
#define access _access
#define chdir _chdir
#endif

static FILE* reserve = NULL;

static int open_files = 0;

void change_dir(const char* dir)
{
    if (chdir(dir) != 0) {
        fprintf(stderr, "Could not set run direcory to '%s'.\n", dir);
        exit(1);
    }

    printf("Run directory set to '%s'.\n", dir);
}

void close_file(FILE* fp)
{
    --open_files;
    fclose(fp);
    open_reserve_file();
}

bool file_exists(const char* dir)
{
    return (access(dir, F_OK) == 0);
}

FILE* open_append_file(const char* filename)
{
    FILE* fp = NULL;
    close_reserve_file();
    if ((fp = fopen(filename, "a")) == NULL) {
        fprintf(stderr, "Couldn't open file '%s' for appending.\n", filename);
        open_reserve_file();
    }
    ++open_files;
    return fp;
}

FILE* open_read_file(const char* filename)
{
    FILE* fp = NULL;
    close_reserve_file();
    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Couldn't open file '%s' for reading.\n", filename);
        open_reserve_file();
    }
    ++open_files;
    return fp;
}

FILE* open_write_file(const char* filename)
{
    FILE* fp = NULL;
    close_reserve_file();
    if ((fp = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "Couldn't open file '%s' for writing.\n", filename);
        open_reserve_file();
    }
    ++open_files;
    return fp;
}

void close_reserve_file()
{
    if (open_files > 0)
        return;
    fclose(reserve);
}

void open_reserve_file()
{
    if (open_files > 0)
        return;

    if ((reserve = fopen(NULL_FILE, "r")) == NULL) {
        fprintf(stderr, "Couldn't open reserve file.\n");
        exit(1);
    }
}


