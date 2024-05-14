////////////////////////////////////////////////////////////////////////////////
// array.c
////////////////////////////////////////////////////////////////////////////////

#include "lox/common.h"

#include "lox/array.h"
#include "lox/object.h"
#include "lox/value.h"

ValueArray* new_obj_array()
{
    ValueArray* array_ = ALLOCATE_OBJ(ValueArray, OBJ_ARRAY);
    init_value_array(array_);
    return array_;
}
