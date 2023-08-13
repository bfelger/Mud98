////////////////////////////////////////////////////////////////////////////////
// strings.h
// 
// Helper functions for string handling.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__STRINGS_H
#define MUD98__STRINGS_H

#include <ctype.h>
#include <inttypes.h>

// All strings in Mud98 are 7-bit POSIX Locale, with the sign bit reserved for 
// special telnet characters. These macros keep 'ctype.h' functions copacetic
// with char* strings under -Wall.

#if defined(__CYGWIN__) || defined(_MSC_VER)
    #define ISALPHA(c) isalpha((unsigned char)c)
    #define ISDIGIT(c) isdigit((unsigned char)c)
    #define ISPRINT(c) isprint((unsigned char)c)
    #define ISSPACE(c) isspace((unsigned char)c)
    #define ISUPPER(c) isupper((unsigned char)c)
#else
    #define ISALPHA(c) isalpha(c)
    #define ISDIGIT(c) isdigit(c)
    #define ISPRINT(c) isprint(c)
    #define ISSPACE(c) isspace(c)
    #define ISUPPER(c) isupper(c)
#endif

// isascii() is marked OB by POSIX, but we still need to test for telnet-
// friendly characters.
#define ISASCII(c) (c >= 0 && c <= 127)

// Handle time_t printf specifiers
#ifdef _MSC_VER
#define TIME_FMT "%" PRIi64
#else
#define TIME_FMT "%ld"
#endif


#endif // !MUD98__STRINGS_H
