////////////////////////////////////////////////////////////////////////////////
// area.c
// Utilities to handle area
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "area.h"

#include "comm.h"
#include "config.h"
#include "db.h"
#include "handler.h"

#include "olc/olc.h"

#include "data/direction.h"

int area_data_perm_count;
// 
ValueArray global_areas = { 0 };
AreaData* area_data_free;



int area_count;
int area_perm_count;
Area* area_free;

Area* new_area(AreaData* area_data)
{
    LIST_ALLOC_PERM(area, Area);

    push(OBJ_VAL(area));

    init_header(&area->header, OBJ_AREA);

    SET_NAME(area, NAME_FIELD(area_data));
    VNUM_FIELD(area) = VNUM_FIELD(area_data);

    SET_NATIVE_FIELD(&area->header, VNUM_FIELD(area), vnum, I32);

    pop();

    area->data = area_data;
    area->empty = true;
    area->owner_list = str_empty;
    area->reset_timer = area->data->reset_thresh;

    return area;
}

void free_area(Area* area)
{
    free_table(&area->header.fields);
    LIST_FREE(area);
}

AreaData* new_area_data()
{
    char buf[MAX_INPUT_LENGTH];

    ENTITY_ALLOC_PERM(area_data, AreaData);

    push(OBJ_VAL(area_data));

    init_header(&area_data->header, OBJ_AREA_DATA);

    SET_NAME(area_data, lox_string(str_empty));

    VNUM_FIELD(area_data) = global_areas.count - 1;
    sprintf(buf, "area%"PRVNUM".are", VNUM_FIELD(area_data));

    pop();

    area_data->area_flags = AREA_ADDED;
    area_data->security = 1;
    area_data->builders = str_empty;
    area_data->credits = str_empty;
    area_data->reset_thresh = 6;
    area_data->file_name = str_dup(buf);

    init_value_array(&area_data->instances);

    return area_data;
}

void free_area_data(AreaData* area_data)
{
    // area_data->name will get GC'd
    free_string(area_data->file_name);
    free_string(area_data->builders);
    free_string(area_data->credits);

    //LIST_FREE(area_data);
}

Area* create_area_instance(AreaData* area_data, bool create_exits)
{
    Area* area = new_area(area_data);
    Value area_val = OBJ_VAL(area);

    push(area_val);
    write_value_array(&area_data->instances, OBJ_VAL(area));
    pop();

    RoomData* room_data;
    FOR_EACH_GLOBAL_ROOM_DATA(room_data) {
        if (room_data->area_data == area_data) {
            // It's okay; it's still being added to area instances
            new_room(room_data, area);
        }
    }

    // Rooms are now all made. Time to populate exits.
    if (create_exits)
        create_instance_exits(area);

    return area;
}

void create_instance_exits(Area* area)
{
    Room* room;
    FOR_EACH_AREA_ROOM(room, area) {
        for (int i = 0; i < DIR_MAX; ++i) {
            if (room->data->exit_data[i])
                room->exit[i] = new_room_exit(room->data->exit_data[i], room);
        }
    }
}

/*
 * OLC
 * Use these macros to load any new area formats that you choose to
 * support on your MUD.  See the load_area format below for
 * a short example.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                \
    if (!str_cmp(word, literal)) {                  \
        field  = value;                             \
        break;                                      \
    }

#define SKEY( string, field )                       \
    if (!str_cmp(word, string)) {                   \
        free_string(field);                         \
        field = fread_string(fp);                   \
        break;                                      \
    }

#define KEYLS(literal, entity, field, value)                                   \
    if (!str_cmp(word, literal)) {                                             \
        (entity)->header.field = (value);                                      \
        SET_LOX_FIELD(&((entity)->header), (entity)->header.field, field);     \
        break;                                                                 \
    }

void load_area(FILE* fp)
{
    AreaData* area_data;
    char* word;
#ifdef _MSC_VER
    int version = 1;
#else
    int __attribute__((unused)) version;
#endif

    area_data = new_area_data();
    VNUM_FIELD(area_data) = global_areas.count;
    area_data->reset_thresh = 6;
    area_data->file_name = str_dup(fpArea);
    area_data->security = 9;

    for (; ; ) {
        word = feof(fp) ? "End" : fread_word(fp);

        switch (UPPER(word[0])) {
        case 'A':
            KEY("AlwaysReset", area_data->always_reset, (bool)fread_number(fp));
            break;
        case 'B':
            SKEY("Builders", area_data->builders);
            break;
        case 'C':
            SKEY("Credits", area_data->credits);
            break;
        case 'E':
            if (!str_cmp(word, "End")) {
                write_value_array(&global_areas, OBJ_VAL(area_data));
                if (global_areas.count > 0)
                    LAST_AREA_DATA->next = area_data;
                area_data->next = NULL;
                current_area_data = area_data;
                return;
            }
            break;
        case 'H':
            KEY("High", area_data->high_range, (LEVEL)fread_number(fp));
            break;
        case 'I':
            KEY("InstType", area_data->inst_type, fread_number(fp));
            break;
        case 'L':
            KEY("Low", area_data->low_range, (LEVEL)fread_number(fp));
            break;
        case 'N':
            KEYLS("Name", area_data, name, fread_lox_string(fp));
            break;
        case 'R':
            KEY("Reset", area_data->reset_thresh, (int16_t)fread_number(fp));
            break;
        case 'S':
            KEY("Security", area_data->security, fread_number(fp));
            KEY("Sector", area_data->sector, fread_number(fp));
            break;
        case 'V':
            if (!str_cmp(word, "VNUMs")) {
                area_data->min_vnum = fread_number(fp);
                area_data->max_vnum = fread_number(fp);
                break;
            }
            KEY("Version", version, fread_number(fp));
            break;
        // End switch
        }
    }
}

Area* get_area_for_player(Mobile* ch, AreaData* area_data)
{
    Area* area = NULL;

    for (int i = 0; i < area_data->instances.count; ++i) {
        area = AS_AREA(area_data->instances.values[i]);
        if (is_name(NAME_STR(ch), area->owner_list))
            return area;
    }

    return NULL;
}
