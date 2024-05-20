////////////////////////////////////////////////////////////////////////////////
// list.c
////////////////////////////////////////////////////////////////////////////////

#include "lox/list.h"

#include "lox/memory.h"
#include "lox/object.h"
#include "lox/table.h"
#include "lox/value.h"

List* new_list()
{
    List* list = ALLOCATE_OBJ(List, OBJ_LIST);
    init_list(list);
    return list;
}

void init_list(List* list)
{
    list->obj.type = OBJ_LIST;
    list->count = 0;
    list->front = NULL;
    list->back = NULL;
}

void free_list(List * list)
{
    while (list->front != NULL)
        list_pop(list);
    init_list(list);
}

static Node* new_node()
{
    Node* node = (Node*)reallocate(NULL, 0, sizeof(Node));
    node->value = NIL_VAL;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

void list_push(List* list, Value value)
{
    Node* node = new_node();
    node->value = value;
    node->next = list->front;

    if (node->next == NULL)
        list->back = node;
    else
        node->next->prev = node;

    list->front = node;

    list->count++;
}

Value list_pop(List* list)
{
    if (list->front == NULL)
        return NIL_VAL;

    Node* old = list->front;
    Value ret = old->value;

    list->front = old->next;
    if (list->front != NULL)
        list->front->prev = NULL;

    reallocate(old, sizeof(Node), 0);

    list->count--;

    return ret;
}

Node* list_find(List* list, Value value)
{
    for (Node* node = list->front; node != NULL; node = node->next)
        if (node->value == value)
            return node;

    return NULL;
}

void list_remove_node(List* list, Node* node)
{
    if (list->front == node)
        list->front = node->next;

    if (list->back == node)
        list->back = node->prev;

    if (node->prev != NULL)
        node->prev->next = node->next;

    if (node->next != NULL)
        node->next->prev = node->prev;

    reallocate(node, sizeof(Node), 0);

    list->count--;
}

void list_remove_value(List* list, Value value)
{
    Node* node = list_find(list, value);
    
    if (node != NULL)
        list_remove_node(list, node);
}

void list_insert_after(List* list, Node* node, Value value)
{
    Node* new_ = new_node();
    new_->value = value;

    new_->next = node->next;
    if (new_->next == NULL)
        list->back = new_;
    else
        new_->next->prev = new_;

    new_->prev = node;
    if (new_->prev == NULL)
        list->front = new_;
    else
        new_->prev->next = new_;

    list->count++;
}

void mark_list(List* list)
{
    for (Node* node = list->front; node != NULL; node = node->next)
        mark_value(node->value);
}
