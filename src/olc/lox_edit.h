////////////////////////////////////////////////////////////////////////////////
// lox_exit.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__OLC__LOX_EDIT_H
#define MUD98__OLC__LOX_EDIT_H

#include <entities/entity.h>
#include <entities/mobile.h>

#include <lox/object.h>

#include <recycle.h>

bool olc_edit_lox(Mobile* ch, char* argument);
void lox_script_append(Mobile* ch, ObjString* script);
void lox_script_add(Mobile* ch, char* argument);
char* prettify_lox_script(char* string);
void olc_display_entity_class_info(Mobile* ch, Entity* entity);

#endif // !MUD98__OLC__LOX_EDIT_H
