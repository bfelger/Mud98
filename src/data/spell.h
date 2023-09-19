////////////////////////////////////////////////////////////////////////////////
// spell.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__SPELL_H
#define MUD98__DATA__SPELL_H

#include "merc.h"
#include "recycle.h"

typedef struct spell_t {
    char* name;
    SpellFunc* spell;
} Spell;

SpellFunc* spell_function(char* argument);
bool spell_fun_read(void* temp, char* arg);
char* spell_fun_str(void* temp);
void spell_list(Buffer* pBuf);
char* spell_name(SpellFunc* spell);

extern const Spell spell_table[];

#endif // !MUD98__DATA__SPELL_H
