////////////////////////////////////////////////////////////////////////////////
// stringutils.c
// 
// String utility functions for Mud98
////////////////////////////////////////////////////////////////////////////////

#include "stringutils.h"

#include <stdlib.h>
#include <string.h>

// Win32-compatible implementation of strndup
// Allocates memory for a copy of the string, limited to n bytes
char* mud98_strndup(const char* s, size_t n)
{
    if (s == NULL) {
        return NULL;
    }
    
    // Find the actual length (could be shorter than n)
    size_t len = 0;
    while (len < n && s[len] != '\0') {
        len++;
    }
    
    // Allocate memory for the copy
    char* result = (char*)malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    // Copy the string
    memcpy(result, s, len);
    result[len] = '\0';
    
    return result;
}