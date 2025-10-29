////////////////////////////////////////////////////////////////////////////////
// list.c
////////////////////////////////////////////////////////////////////////////////

#include "lox/list.h"

#include "lox/memory.h"
#include "lox/object.h"
#include "lox/table.h"
#include "lox/value.h"

static Node* node_free = NULL;
static size_t node_count = 0;
static size_t node_perm_count = 0;

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
    Node* node;

    if (node_free != NULL) {
        node = node_free;
        node_free = node_free->next;
    }
    else {
        node = (Node*)reallocate_nogc(NULL, 0, sizeof(Node));
        node_perm_count++;
    }

    node->value = NIL_VAL;
    node->next = NULL;
    node->prev = NULL;
    node_count++;
    return node;
}

Node* list_push(List* list, Value value)
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

    return node;
}

Node* list_push_back(List* list, Value value)
{
    Node* node = new_node();
    node->value = value;
    node->prev = list->back;

    if (node->prev == NULL)
        list->front = node;
    else
        node->prev->next = node;

    list->back = node;

    list->count++;

    return node;
}

Value list_pop(List* list)
{
    if (list == NULL || list->front == NULL)
        return NIL_VAL;

    Node* old = list->front;
    Value ret = old->value;

    list->front = old->next;
    if (list->front != NULL)
        list->front->prev = NULL;

    //reallocate(old, sizeof(Node), 0);
    old->next = node_free;
    node_free = old;
    node_count--;

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
    if (node == NULL || list == NULL)
        return;

    if (list->front == node)
        list->front = node->next;
    if (list->back == node)
        list->back = node->prev;

    if (node->prev != NULL)
        node->prev->next = node->next;
    if (node->next != NULL)
        node->next->prev = node->prev;

    //reallocate(node, sizeof(Node), 0);
    node_count--;
    list->count--;
    node->next = node_free;
    node_free = node;
}

void list_remove_value(List* list, Value value)
{
    Node* node = list_find(list, value);
    
    if (node != NULL)
        list_remove_node(list, node);
}

Node* list_insert_after(List* list, Node* node, Value value)
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

    return new_;
}

void mark_list(List* list)
{
    mark_object((Obj*)list);
    for (Node* node = list->front; node != NULL; node = node->next)
        mark_value(node->value);
}
