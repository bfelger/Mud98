/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St�rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

#pragma once
#ifndef MUD98__MERC_H
#define MUD98__MERC_H

#define MUD_NAME "Mud98"
#define MUD_VER "0.90"

#include "rng.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef _MSC_VER
#include <stddef.h>
#endif

#define AREA_VERSION        3

////////////////////////////////////////////////////////////////////////////////

// Expand when you run out of bits
#define FLAGS               int32_t

// Used for flags and other bit fields. Unsigned with cast back to signed so we 
// can use BIT(31) for enums (which must be signed) under -pedantic.
#define BIT(x)              (FLAGS)(1u << (x))

#define NO_FLAG             (FLAGS)BIT(31)

#define SHORT_FLAGS         int16_t

#define LEVEL               int16_t

#define SKNUM               int16_t

// If you want to change the type for VNUM's, this is where you do it.
#define MAX_VNUM            INT32_MAX
#define VNUM                int32_t
#define PRVNUM              PRId32
#define STRTOVNUM(s)        (VNUM)strtol(s, NULL, 0)
#define VNUM_NONE           -1

////////////////////////////////////////////////////////////////////////////////
// Target Info
////////////////////////////////////////////////////////////////////////////////

typedef enum skill_target_t {
    SKILL_TARGET_ALL                = -1,   // Used for display all to user
    SKILL_TARGET_IGNORE             = 0,
    SKILL_TARGET_CHAR_OFFENSIVE     = 1,
    SKILL_TARGET_CHAR_DEFENSIVE     = 2,
    SKILL_TARGET_CHAR_SELF          = 3,
    SKILL_TARGET_OBJ_INV            = 4,
    SKILL_TARGET_OBJ_CHAR_DEF       = 5,
    SKILL_TARGET_OBJ_CHAR_OFF       = 6,
} SkillTarget;

typedef enum spell_target_t {
    SPELL_TARGET_CHAR               = 0,
    SPELL_TARGET_OBJ                = 1,
    SPELL_TARGET_ROOM               = 2,
    SPELL_TARGET_NONE               = 3,
} SpellTarget;

////////////////////////////////////////////////////////////////////////////////
// Func Helpers
////////////////////////////////////////////////////////////////////////////////

typedef struct mobile_t Mobile;

typedef void DoFunc(Mobile* ch, char* argument);
typedef bool SpecFunc(Mobile* ch);
typedef void SpellFunc(SKNUM sn, LEVEL level, Mobile* ch, void* vo, SpellTarget target);

#define DECLARE_DO_FUN( fun )       DoFunc fun
#define DECLARE_SPEC_FUN( fun )     SpecFunc fun
#define DECLARE_SPELL_FUN( fun )    SpellFunc fun

////////////////////////////////////////////////////////////////////////////////

// String and memory management parameters.
#define MAX_KEY_HASH        1024
#define MAX_STRING_LENGTH   4608
#define MAX_INPUT_LENGTH    256
#define PAGELEN             22

#define MSL MAX_STRING_LENGTH
#define MIL MAX_INPUT_LENGTH

#define INPUT_BUFFER_SIZE   8192

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */

#define MAX_IN_GROUP        15
#define MAX_ALIAS           5
#define MAX_THEMES          5
#define MAX_CLAN            3
#define MAX_LEVEL           (LEVEL)60
#define LEVEL_HERO          (MAX_LEVEL - 9)
#define LEVEL_IMMORTAL      (MAX_LEVEL - 8)

#define PULSE_PER_SECOND    4
#define PULSE_VIOLENCE      (3 * PULSE_PER_SECOND)
#define PULSE_MOBILE        (4 * PULSE_PER_SECOND)
#define PULSE_MUSIC         (6 * PULSE_PER_SECOND)
#define PULSE_TICK          (60 * PULSE_PER_SECOND)
#define PULSE_AREA          (120 * PULSE_PER_SECOND)

#define IMPLEMENTOR         MAX_LEVEL
#define CREATOR             (MAX_LEVEL - 1)
#define SUPREME             (MAX_LEVEL - 2)
#define DEITY               (MAX_LEVEL - 3)
#define GOD                 (MAX_LEVEL - 4)
#define IMMORTAL            (MAX_LEVEL - 5)
#define DEMI                (MAX_LEVEL - 6)
#define ANGEL               (MAX_LEVEL - 7)
#define AVATAR              (MAX_LEVEL - 8)
#define HERO                LEVEL_HERO

// Utility macros.
#define IS_VALID(data)       ((data) != NULL && (data)->valid)
#define VALIDATE(data)       ((data)->valid = true)
#define INVALIDATE(data)     ((data)->valid = false)
#define UMIN(a, b)           ((a) < (b) ? (a) : (b))
#define UMAX(a, b)           ((a) > (b) ? (a) : (b))

#define COPPER_PER_SILVER     100
#define SILVER_PER_GOLD       100
#define COPPER_PER_GOLD       (COPPER_PER_SILVER * SILVER_PER_GOLD)
#define URANGE(a, b, c)      ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)             ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c))
#define UPPER(c)             ((c) >= 'a' && (c) <= 'z' ? (c) + 'A' - 'a' : (c))
#define IS_SET(flag, bit)    ((flag) & (bit))
#define SET_BIT(var, bit)    ((var) |= (bit))
#define REMOVE_BIT(var, bit) ((var) &= ~(bit))
#define TOGGLE_BIT(var, bit) ((var) ^= (bit))
#define IS_NULLSTR(str)		 ((str) == NULL || (str)[0] == '\0')
#define CHECKNULLSTR(str)    ((str) == NULL ? "" : (str))
#define CH(d)		         ((d)->original ? (d)->original : (d)->character)
#define BETWEEN(min,num,max) (((min) < (num)) && ((num) < (max)))
#define BETWEEN_I(x,y,z)     (((x) <= (y)) && ((y) <= (z)))
#define CHECK_POS(a, b, c)   \
    { \
        (a) = (b); \
        if ((a) < 0) \
            bug( "CHECK_POS : " c " == %d < 0", a ); \
    }
#define ARRAY_COPY(array1, array2, len) \
    { \
        int _xxx_; \
        for (_xxx_ = 0; _xxx_ < len; _xxx_++) \
            array1[_xxx_] = array2[_xxx_]; \
    }
#define READ_ARG(arg)       (argument = one_argument(argument, (arg)))
#define NEXT_LINK(n)        ((n) = (n)->next)
#define FOR_EACH(i, l)      for ((i) = (l); (i) != NULL; NEXT_LINK(i))

#define ORDERED_INSERT(T, i, l, f)                                             \
    if (!(l) || i->f < (l)->f) {                                               \
        i->next = l;                                                           \
        l = i;                                                                 \
    } else {                                                                   \
        T* temp_ ## i;                                                         \
        FOR_EACH(temp_ ## i, (l)) {                                            \
            if (!temp_ ## i->next || i->f < temp_ ## i->next->f) {             \
                i->next = temp_ ## i->next;                                    \
                temp_## i->next = i;                                           \
                break;                                                         \
            }                                                                  \
        }                                                                      \
    }
#define UNORDERED_REMOVE(T, i, l, f, v)                                        \
    if ((l)) {                                                                 \
        if ((l)->f == v) {                                                     \
            (i) = (l);                                                         \
            NEXT_LINK(l);                                                      \
        } else {                                                               \
            T* temp_ ## i;                                                     \
            FOR_EACH(temp_ ## i, (l)) {                                        \
                if (!temp_ ## i->next) {                                       \
                    break;                                                     \
                } else if (temp_ ## i->next->f == v) {                         \
                    (i) = temp_ ## i->next;                                    \
                    temp_ ## i->next = (i)->next;                              \
                    break;                                                     \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }
#define UNORDERED_REMOVE_ALL(T, i, l, f, v)                                    \
    if ((l)) {                                                                 \
        if ((l)->f == v) {                                                     \
            (i) = (l);                                                         \
            NEXT_LINK(l);                                                      \
        } else {                                                               \
            T* temp_ ## i;                                                     \
            FOR_EACH(temp_ ## i, (l)) {                                        \
                if (!temp_ ## i->next) {                                       \
                    break;                                                     \
                } else if (temp_ ## i->next->f == v) {                         \
                    (i) = temp_ ## i->next;                                    \
                    temp_ ## i->next = (i)->next;                              \
                    break;                                                     \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }
#define ORDERED_GET(T, i, l, f, v)                                             \
    {                                                                          \
        T* temp_ ## i;                                                         \
        FOR_EACH(temp_ ## i, (l)) {                                            \
            if (temp_ ## i->f == v)  {                                         \
                i = temp_ ## i;                                                \
                break;                                                         \
            } else if (temp_ ## i->f > v)                                      \
                break;                                                         \
        }                                                                      \
    }

#define IS_PERM_STRING(str) \
    (str == &str_empty[0] || (str >= string_space && str < top_string))

#ifdef _MSC_VER
    #define strdup _strdup
    #define strcasecmp _stricmp
#endif

////////////////////////////////////////////////////////////////////////////////
// Global Vars
////////////////////////////////////////////////////////////////////////////////

// main.c
int get_uptime(void);

extern bool merc_down;                      // Shutdown
extern bool wizlock;                        // Game is wizlocked
extern bool newlock;                        // Game is newlocked
extern time_t current_time;
extern bool events_enabled;

// db.c
extern char str_empty[1];

#endif // !MUD98__MERC_H
