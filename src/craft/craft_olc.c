////////////////////////////////////////////////////////////////////////////////
// craft_olc.c
// Shared OLC helper functions for crafting material lists
////////////////////////////////////////////////////////////////////////////////

#include "craft_olc.h"
#include "craft.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "recycle.h"
#include "stringbuffer.h"

#include <entities/object.h>
#include <entities/obj_prototype.h>
#include <entities/area.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

////////////////////////////////////////////////////////////////////////////////
// Material List Management
////////////////////////////////////////////////////////////////////////////////

bool craft_olc_add_mat(VNUM** list, int16_t* count, VNUM vnum, Mobile* ch)
{
    ObjPrototype* proto = get_object_prototype(vnum);
    if (!proto) {
        send_to_char("That object doesn't exist.\n\r", ch);
        return false;
    }
    
    if (proto->item_type != ITEM_MAT) {
        send_to_char("That object is not a crafting material (ITEM_MAT).\n\r", ch);
        return false;
    }
    
    // Check for duplicate
    for (int i = 0; i < *count; i++) {
        if ((*list)[i] == vnum) {
            send_to_char("That material is already in the list.\n\r", ch);
            return false;
        }
    }
    
    // Reallocate and add
    VNUM* new_list = realloc(*list, sizeof(VNUM) * (size_t)(*count + 1));
    if (!new_list) {
        send_to_char("Memory allocation failed.\n\r", ch);
        return false;
    }
    
    *list = new_list;
    (*list)[*count] = vnum;
    (*count)++;
    
    printf_to_char(ch, "Added %s [%d].\n\r", proto->short_descr, vnum);
    return true;
}

bool craft_olc_remove_mat(VNUM** list, int16_t* count, const char* arg, Mobile* ch)
{
    if (*count == 0) {
        send_to_char("The material list is empty.\n\r", ch);
        return false;
    }
    
    int index = -1;
    
    // Check if arg is a number (could be index or VNUM)
    if (isdigit(arg[0])) {
        int num = atoi(arg);
        
        // First check if it's a 1-based index (small number)
        if (num > 0 && num <= *count) {
            index = num - 1;  // Convert to 0-based
        }
        else {
            // Try as VNUM
            for (int i = 0; i < *count; i++) {
                if ((*list)[i] == num) {
                    index = i;
                    break;
                }
            }
        }
    }
    
    if (index < 0) {
        send_to_char("Material not found in list.\n\r", ch);
        return false;
    }
    
    VNUM removed_vnum = (*list)[index];
    ObjPrototype* proto = get_object_prototype(removed_vnum);
    
    // Shift remaining elements
    for (int i = index; i < *count - 1; i++) {
        (*list)[i] = (*list)[i + 1];
    }
    (*count)--;
    
    // Shrink or free the array
    if (*count == 0) {
        free(*list);
        *list = NULL;
    }
    else {
        VNUM* new_list = realloc(*list, sizeof(VNUM) * (size_t)(*count));
        if (new_list) {
            *list = new_list;
        }
    }
    
    if (proto) {
        printf_to_char(ch, "Removed %s [%d].\n\r", proto->short_descr, removed_vnum);
    }
    else {
        printf_to_char(ch, "Removed VNUM %d.\n\r", removed_vnum);
    }
    
    return true;
}

void craft_olc_clear_mats(VNUM** list, int16_t* count)
{
    if (*list) {
        free(*list);
        *list = NULL;
    }
    *count = 0;
}

void craft_olc_show_mats(VNUM* list, int16_t count, Mobile* ch, const char* label)
{
    if (count == 0 || !list) {
        printf_to_char(ch, "%s: (none)\n\r", label);
        return;
    }
    
    StringBuffer* sb = sb_new();
    sb_appendf(sb, "%s:\n\r", label);
    
    for (int i = 0; i < count; i++) {
        ObjPrototype* proto = get_object_prototype(list[i]);
        if (proto) {
            sb_appendf(sb, "  [%d] %s [%d] (%s)\n\r", 
                i + 1,
                proto->short_descr, 
                list[i],
                craft_mat_type_name(proto->craft_mat.mat_type));
        }
        else {
            sb_appendf(sb, "  [%d] INVALID VNUM %d\n\r", i + 1, list[i]);
        }
    }
    
    send_to_char(sb_string(sb), ch);
    sb_free(sb);
}

void craft_olc_list_mats(AreaData* area, CraftMatType filter, Mobile* ch)
{
    StringBuffer* sb = sb_new();
    int found = 0;
    
    if (filter == MAT_NONE) {
        sb_append(sb, "Available crafting materials");
    }
    else {
        sb_appendf(sb, "Available %s materials", craft_mat_type_name(filter));
    }
    
    if (area) {
        sb_appendf(sb, " in %s", area->credits);
    }
    sb_append(sb, ":\n\r");
    
    // Iterate through all object prototypes
    for (VNUM vnum = (area ? area->min_vnum : 0); 
         vnum <= (area ? area->max_vnum : 65535); 
         vnum++) {
        ObjPrototype* proto = get_object_prototype(vnum);
        if (!proto) continue;
        if (proto->item_type != ITEM_MAT) continue;
        if (filter != MAT_NONE && proto->craft_mat.mat_type != (int)filter) continue;
        
        sb_appendf(sb, "  [%5d] %-30s (%s)\n\r",
            vnum,
            proto->short_descr,
            craft_mat_type_name(proto->craft_mat.mat_type));
        found++;
        
        // Limit output to prevent spam
        if (found >= 50) {
            sb_append(sb, "  ... (list truncated, use filter or area to narrow results)\n\r");
            break;
        }
    }
    
    if (found == 0) {
        sb_append(sb, "  (none found)\n\r");
    }
    
    page_to_char(sb_string(sb), ch);
    sb_free(sb);
}
