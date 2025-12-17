////////////////////////////////////////////////////////////////////////////////
// array.h
////////////////////////////////////////////////////////////////////////////////

// TODO: Take a look at C23's new generic features to see if they can
// simplify or replace this header. Also consider replacing with Lox arrays.

#pragma once
#ifndef MUD98__ARRAY_H
#define MUD98__ARRAY_H

// This header defines a series of macros that attempt to approximate "generic"
// collections in other languages.
//
// This example creates an array of SomeType and the functions needed to utilize
// it:
//     typdef struct some_type_t SomeType;
//     DEFINE_ARRAY(SomeType, some_default_value);
//
// This is how you declare an array of SomeType and initialize it to empty:
//     ARRAY(SomeType) some_items;
//     INIT_ARRAY(some_items, SomeType)
//
// To get a new value from the end of the array (resizing it as necessary):
//     SomeItem* item = CREATE_ELEM(&some_items);
//

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// When upsizing arrays, increase size by this magnitutde:
#define ARRAY_GROWTH    2

// Default capacity of new arrays:
#define ARRAY_START     8

#define DECLARE_ARRAY(_Type)                                                   \
    typedef struct _Type ## _array_t {                                         \
        size_t count;                                                          \
        size_t capacity;                                                       \
        _Type* elems;                                                          \
        _Type default_val;                                                     \
        _Type* (*create_func)(struct _Type ## _array_t* arr);                  \
    } _Type ## Array;                                                          \
    extern _Type default_ ## _Type ## _val;                                    \
    _Type* new_ ## _Type(_Type ## Array* arr);                                 \

#define DEFINE_ARRAY(_Type, _DefaultVal)                                       \
    _Type default_ ## _Type ## _val = _DefaultVal;                             \
    _Type* new_ ## _Type(_Type ## Array* arr) {                                \
        if (arr->elems == NULL) {                                              \
            arr->capacity = ARRAY_START;                                       \
            arr->elems = (_Type*)malloc(sizeof(_Type) * arr->capacity);        \
            if (arr->elems == NULL) {                                          \
                fprintf(stderr, "Not enough free memeory to create array!\n"); \
                exit(1);                                                       \
            }                                                                  \
        } else if (arr->count == arr->capacity) {                              \
            arr->capacity = (size_t)((double)arr->capacity * ARRAY_GROWTH);    \
            _Type* temp = (_Type*)realloc(arr->elems,                          \
                sizeof(_Type) * arr->capacity);                                \
            if (temp == NULL) {                                                \
                fprintf(stderr, "Not enough free memeory to resize array!\n"); \
                exit(1);                                                       \
            }                                                                  \
            arr->elems = temp;                                                 \
        }                                                                      \
        assert(arr->elems != NULL);                                            \
        _Type* elem = &arr->elems[arr->count++];                               \
        *elem = arr->default_val;                                              \
        return elem;                                                           \
    }

#define CREATE_ELEM(arr) (*((arr).create_func))(&arr)

#define ARRAY(_Type) _Type ## Array

#define GET_ELEM(arr, idx)                                                     \
    (((size_t)(idx) < (arr)->count) ? (arr)->elems[(idx)] : (arr)->default_val)

#define SET_ELEM(arr, idx, val)                                                \
    while ((size_t)idx >= arr.count) {                                         \
        CREATE_ELEM(arr);                                                      \
    }                                                                          \
    arr.elems[idx] = val;

#define INIT_ARRAY(arr, _Type)                                                 \
    (arr).count = 0;                                                           \
    (arr).capacity = 0;                                                        \
    (arr).elems = NULL;                                                        \
    (arr).default_val = default_ ## _Type ## _val;                             \
    (arr).create_func = &new_ ## _Type;

////////////////////////////////////////////////////////////////////////////////

// Generic utilities not necessarily tied to any of the macros above

#define SWAP(a, b, temp)    temp = a; a = b; b = temp;
#define SORT_ARRAY(Type, arr, arr_len, i_lt_lo_test, i_gt_hi_test)             \
{                                                                              \
    int first = 0;                                                             \
    int last = arr_len - 1;                                                    \
    Type temp;                                                                 \
    while (first < last) {                                                     \
        int hi = last;                                                         \
        int lo = first;                                                        \
        for (int i = first; i <= last; ++i) {                                  \
            if (i_lt_lo_test)                                                  \
                lo = i;                                                        \
            if (i_gt_hi_test)                                                  \
                hi = i;                                                        \
        }                                                                      \
        if (hi != last) {                                                      \
            SWAP(arr[hi], arr[last], temp)                                     \
            if (lo == last)                                                    \
                lo = hi;                                                       \
        }                                                                      \
        if (lo != first) {                                                     \
            SWAP(arr[lo], arr[first], temp)                                    \
        }                                                                      \
        --last;                                                                \
        ++first;                                                               \
    }                                                                          \
}

#endif // !MUD98__ARRAY_H
