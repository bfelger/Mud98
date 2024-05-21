////////////////////////////////////////////////////////////////////////////////
// list.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_list_h
#define clox_list_h

#include "lox/common.h"
#include "lox/object.h"
#include "lox/value.h"

typedef struct Node {
    Value value;
    struct Node* next;
    struct Node* prev;
} Node;

typedef struct {
    Obj obj;
    int count;
    Node* front;
    Node* back;
} List;

#define FOR_EACH_LINK(val, list, type) \
    for (Node* val##_node = list->front; val##_node != NULL; val##_node = val##_node->next) \
        if ((val = AS_##type(val##_node->value)) != NULL)

List* new_list();
void init_list(List* list);
void free_list(List* list);
void list_push(List* list, Value value);
void list_push_back(List* list, Value value);
Value list_pop(List* list);
Node* list_find(List* list, Value value);
void list_remove_node(List* list, Node* node);
void list_remove_value(List* list, Value value);
void list_insert_after(List* list, Node* node, Value value);
void mark_list(List* list);

#endif // !clox_list_h
