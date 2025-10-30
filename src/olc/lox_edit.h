////////////////////////////////////////////////////////////////////////////////
// lox_exit.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__OLC__LOX_EDIT_H
#define MUD98__OLC__LOX_EDIT_H

#include <entities/mobile.h>

void lox_script_append(Mobile* ch, char** pScript);
void lox_script_add(Mobile* ch, char* argument);
char* prettify_lox_script(char* string);

#endif // !MUD98__OLC__LOX_EDIT_H
