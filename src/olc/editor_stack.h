////////////////////////////////////////////////////////////////////////////////
// olc/editor_stack.h
//
// Editor stack for nested OLC editors (e.g., aedit -> loot -> table)
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__OLC__EDITOR_STACK_H
#define MUD98__OLC__EDITOR_STACK_H

#include <stdbool.h>
#include <stdint.h>

// Maximum depth of nested editors (should be plenty for practical use)
#define MAX_EDITOR_DEPTH 8

typedef struct descriptor_t Descriptor;

// Editor type enumeration
typedef enum editor_t {
    ED_NONE         = 0,
    ED_AREA         = 1,
    ED_ROOM         = 2,
    ED_OBJECT       = 3,
    ED_MOBILE       = 4,
    ED_PROG         = 5,
    ED_CLAN         = 6,
    ED_RACE         = 7,
    ED_SOCIAL       = 8,
    ED_SKILL        = 9,
    ED_CMD          = 10,
    ED_GROUP        = 11,
    ED_CLASS        = 12,
    ED_HELP         = 13,
    ED_QUEST        = 14,
    ED_THEME        = 15,
    ED_TUTORIAL     = 16,
    ED_SCRIPT       = 17,
    ED_LOOT         = 18,
    ED_STRING       = 19,       // String editing mode (pEdit = char**)
    ED_LOX_SCRIPT   = 20,       // Lox script editing mode (pEdit = ObjString*)
} EditorType;

// Single editor context on the stack
typedef struct editor_frame_t {
    EditorType editor;      // Editor type
    uintptr_t pEdit;        // Editor context pointer
} EditorFrame;

// The editor stack itself
typedef struct editor_stack_t {
    EditorFrame frames[MAX_EDITOR_DEPTH];
    int8_t depth;           // Current stack depth (0 = empty)
} EditorStack;

// Initialize an editor stack (sets depth to 0)
void editor_stack_init(EditorStack* stack);

// Push a new editor onto the stack
// Returns true if successful, false if stack is full
bool editor_stack_push(EditorStack* stack, EditorType editor, uintptr_t pEdit);

// Pop the top editor from the stack
// Returns true if successful, false if stack is empty
bool editor_stack_pop(EditorStack* stack);

// Get the current (top) editor frame
// Returns NULL if stack is empty
EditorFrame* editor_stack_top(EditorStack* stack);

// Get the editor frame at a specific depth (0 = bottom/first)
// Returns NULL if index out of range
EditorFrame* editor_stack_at(EditorStack* stack, int index);

// Check if stack has any editors
bool editor_stack_empty(EditorStack* stack);

// Get current depth
int editor_stack_depth(EditorStack* stack);

// Clear the entire stack
void editor_stack_clear(EditorStack* stack);

// Update the pEdit of the current (top) editor frame
// Returns true if successful, false if stack is empty
bool editor_stack_set_pEdit(EditorStack* stack, uintptr_t pEdit);

#endif // !MUD98__OLC__EDITOR_STACK_H
