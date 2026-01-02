////////////////////////////////////////////////////////////////////////////////
// olc/editor_stack.c
//
// Editor stack implementation for nested OLC editors
////////////////////////////////////////////////////////////////////////////////

#include "editor_stack.h"

#include <stddef.h>
#include <string.h>

void editor_stack_init(EditorStack* stack)
{
    if (stack == NULL) return;
    memset(stack->frames, 0, sizeof(stack->frames));
    stack->depth = 0;
}

bool editor_stack_push(EditorStack* stack, EditorType editor, uintptr_t pEdit)
{
    if (stack == NULL) return false;
    if (stack->depth >= MAX_EDITOR_DEPTH) return false;
    
    stack->frames[stack->depth].editor = editor;
    stack->frames[stack->depth].pEdit = pEdit;
    stack->depth++;
    return true;
}

bool editor_stack_pop(EditorStack* stack)
{
    if (stack == NULL) return false;
    if (stack->depth <= 0) return false;
    
    stack->depth--;
    stack->frames[stack->depth].editor = 0;
    stack->frames[stack->depth].pEdit = 0;
    return true;
}

EditorFrame* editor_stack_top(EditorStack* stack)
{
    if (stack == NULL) return NULL;
    if (stack->depth <= 0) return NULL;
    return &stack->frames[stack->depth - 1];
}

EditorFrame* editor_stack_at(EditorStack* stack, int index)
{
    if (stack == NULL) return NULL;
    if (index < 0 || index >= stack->depth) return NULL;
    return &stack->frames[index];
}

bool editor_stack_empty(EditorStack* stack)
{
    if (stack == NULL) return true;
    return stack->depth <= 0;
}

int editor_stack_depth(EditorStack* stack)
{
    if (stack == NULL) return 0;
    return stack->depth;
}

void editor_stack_clear(EditorStack* stack)
{
    if (stack == NULL) return;
    editor_stack_init(stack);
}

bool editor_stack_set_pEdit(EditorStack* stack, uintptr_t pEdit)
{
    EditorFrame* frame = editor_stack_top(stack);
    if (frame == NULL) return false;
    frame->pEdit = pEdit;
    return true;
}
