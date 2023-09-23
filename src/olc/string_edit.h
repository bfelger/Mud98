////////////////////////////////////////////////////////////////////////////////
// string_edit.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__STRING_EDIT_H
#define MUD98__STRING_EDIT_H

#include "entities/char_data.h"

void string_edit(CharData* ch, char** pString);
void string_append(CharData* ch, char** pString);
char* string_replace(char* orig, char* old, char* new_str);
void string_add(CharData* ch, char* argument);
char* format_string(char* oldstring /*, bool fSpace */);
char* first_arg(char* argument, char* arg_first, bool fCase);
char* string_unpad(char* argument);
char* string_proper(char* argument);

#endif // !MUD98__STRING_EDIT_H
