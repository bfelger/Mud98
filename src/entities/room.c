////////////////////////////////////////////////////////////////////////////////
// room.c
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

#include "room.h"

#include "room_exit.h"
#include "event.h"
#include "extra_desc.h"
#include "reset.h"

#include <lox/lox.h>
#include <lox/table.h>
#include <lox/function.h>
#include <lox/memory.h>
#include <lox/vm.h>

#include <comm.h>
#include <db.h>
#include <handler.h>
#include <lookup.h>

int room_count;
int room_perm_count;
Room* room_free;

int room_data_count;
int room_data_perm_count;
RoomData* room_data_free;

Table global_rooms;

VNUM top_vnum_room;

Room* new_room(RoomData* room_data, Area* area)
{
    LIST_ALLOC_PERM(room, Room);

    push(OBJ_VAL(room));

    init_header(&room->header, OBJ_ROOM);

    init_list(&room->mobiles);
    SET_LOX_FIELD(&room->header, &room->mobiles, mobiles);

    init_list(&room->objects);
    SET_LOX_FIELD(&room->header, &room->objects, objects);

    SET_NAME(room, NAME_FIELD(room_data));

    VNUM_FIELD(room) = VNUM_FIELD(room_data);
    SET_NATIVE_FIELD(&room->header, room->header.vnum, vnum, I32);

    room->area = area;
    SET_LOX_FIELD(&room->header, room->area, area);

    table_set_vnum(&area->rooms, VNUM_FIELD(room), OBJ_VAL(room));

    room->data = room_data;

    if (room_data->header.klass != NULL) {
        room->header.klass = room_data->header.klass;
        init_entity_class((Entity*)room);
    }

    room->header.events = room_data->header.events;
    room->header.event_triggers = room_data->header.event_triggers;
    
    list_push_back(&room_data->instances, OBJ_VAL(room));

    pop();

    return room;
}

void free_room(Room* room)
{
    Area* area = room->area;

    list_remove_value(&room->data->instances, OBJ_VAL(room));

    table_delete_vnum(&area->rooms, VNUM_FIELD(room));

    LIST_FREE(room);
}

RoomData* new_room_data()
{
    LIST_ALLOC_PERM(room_data, RoomData);

    push(OBJ_VAL(room_data));

    init_header(&room_data->header, OBJ_ROOM_DATA);

    init_list(&room_data->instances);
    SET_LOX_FIELD(&room_data->header, &room_data->instances, instances);

    SET_NAME(room_data, lox_empty_string);
    room_data->description = &str_empty[0];
    room_data->owner = &str_empty[0];
    room_data->heal_rate = 100;
    room_data->mana_rate = 100;

    pop();

    return room_data;
}

void free_room_data(RoomData* room_data)
{
    ExtraDesc* extra;
    Reset* reset;
    int i;

    free_string(room_data->description);
    free_string(room_data->owner);

    for (i = 0; i < DIR_MAX; i++)
        free_room_exit_data(room_data->exit_data[i]);

    while((extra = room_data->extra_desc) != NULL) {
        NEXT_LINK(room_data->extra_desc);
        free_extra_desc(extra);
    }

    while((reset = room_data->reset_first) != NULL) {
        NEXT_LINK(room_data->reset_first);
        free_reset(reset);
    }

    free_list(&room_data->instances);

    LIST_FREE(room_data);
    return;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
RoomData* get_room_data(VNUM vnum)
{
    RoomData* room_data = NULL;

    Value val;
    table_get_vnum(&global_rooms, vnum, &val);

    if (IS_ROOM_DATA(val))
        room_data = AS_ROOM_DATA(val);

    if (!room_data && fBootDb) {
        bug("get_room_data: bad vnum %"PRVNUM".", vnum);
        exit(1);
    }

    return room_data;
}

Room* get_room(Area* search_context, VNUM vnum)
{
    Area* search_area = NULL;
    RoomData* room_data = get_room_data(vnum);

    if (!room_data) {
        if (fBootDb) {
            bug("get_room: No RoomData for %"PRVNUM".", vnum);
            exit(1);
        }
        return NULL;
    }

    if (search_context && room_data->area_data == search_context->data) {
        // The current area and the given VNUM share the same AreaData; find the 
        // room in the same instance.
        search_area = search_context;
    }
    else if (room_data->area_data->inst_type == AREA_INST_SINGLE) {
        // The target area isn't instanced; it only has one Area object.
        search_area = AS_AREA(room_data->area_data->instances.front->value);
    }
    else {
        return NULL;
    }
    
    Room* room = NULL;
    Value val;

    table_get_vnum(&search_area->rooms, vnum, &val);

    if (IS_ROOM(val))
        room = AS_ROOM(val);

    if (fBootDb && !room) {
        bug("get_room: No Room in Area instance for %"PRVNUM".", vnum);
        exit(1);
    }

    return room;
}

Room* get_room_for_player(Mobile* ch, VNUM vnum)
{
    Area* area = ch->in_room ? ch->in_room->area : NULL;
    // Look for existing room in current area or another single-instance area
    Room* room = get_room(area, vnum);

    if (room)
        return room;

    RoomData* room_data = get_room_data(vnum);
    if (!room_data || room_data->area_data->inst_type != AREA_INST_MULTI)
        return NULL;

    // Look for an existing instance owned by the player
    area = get_area_for_player(ch, room_data->area_data);

    if (area) {
        // There is an instance. Find the vnum for the room
        return get_room(area, vnum);
    }

    // TODO: Check for party members and use theirs, as well, adding the new
    // name to the owner_list of the existing instance.

    // No instance exists. We have to make one.
    printf_log("Creating new instance '%s' for %s.", 
        NAME_STR(room_data->area_data), NAME_STR(ch));
    area = create_area_instance(room_data->area_data, true);
    INIT_BUF(buf, MSL);
    addf_buf(buf, "%s %s", NAME_STR(ch), area->owner_list);
    RESTRING(area->owner_list, BUF(buf));
    reset_area(area);
    free_buf(buf);
    return get_room(area, vnum);
}

// Boot routines

void load_rooms(FILE* fp)
{
    RoomData* room_data;

    if (global_areas.count == 0) {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;
        int door;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_rooms: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0) break;

        fBootDb = false;
        if (get_room_data(vnum) != NULL) {
            bug("Load_rooms: vnum %"PRVNUM" duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        room_data = new_room_data();
        push(OBJ_VAL(room_data));
        room_data->owner = str_dup("");
        room_data->area_data = LAST_AREA_DATA;
        VNUM_FIELD(room_data) = vnum;
        SET_NAME(room_data, fread_lox_string(fp));
        room_data->description = fread_string(fp);
        /* Area number */ fread_number(fp);
        room_data->room_flags = fread_flag(fp);
        /* horrible hack */
        if (3000 <= vnum && vnum < 3400)
            SET_BIT(room_data->room_flags, ROOM_LAW);
        room_data->sector_type = (int16_t)fread_number(fp);

        /* defaults */
        room_data->heal_rate = 100;
        room_data->mana_rate = 100;

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'S')
                break;

            if (letter == 'H') /* healing room */
                room_data->heal_rate = (int16_t)fread_number(fp);

            else if (letter == 'M') /* mana room */
                room_data->mana_rate = (int16_t)fread_number(fp);

            else if (letter == 'C') /* clan */
            {
                if (room_data->clan) {
                    bug("Load_rooms: duplicate clan fields.", 0);
                    exit(1);
                }
                room_data->clan = (int16_t)clan_lookup(fread_string(fp));
            }

            else if (letter == 'D') {
                int locks;

                door = fread_number(fp);
                if (door < 0 || door >= DIR_MAX) {
                    bug("Fread_rooms: vnum %"PRVNUM" has bad door number.", vnum);
                    exit(1);
                }

                RoomExitData* room_exit_data = new_room_exit_data();
                room_exit_data->description = fread_string(fp);
                room_exit_data->keyword = fread_string(fp);
                locks = fread_number(fp);
                room_exit_data->key = (int16_t)fread_number(fp);
                room_exit_data->to_vnum = fread_number(fp);
                room_exit_data->orig_dir = door;

                switch (locks) {
                case 1:
                    room_exit_data->exit_reset_flags = EX_ISDOOR;
                    break;
                case 2:
                    room_exit_data->exit_reset_flags = EX_ISDOOR | EX_PICKPROOF;
                    break;
                case 3:
                    room_exit_data->exit_reset_flags = EX_ISDOOR | EX_NOPASS;
                    break;
                case 4:
                    room_exit_data->exit_reset_flags = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                    break;
                }

                room_data->exit_data[door] = room_exit_data;
            }
            else if (letter == 'E') {
                ExtraDesc* ed;
                ed = new_extra_desc();
                if (ed == NULL)
                    exit(1);
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ADD_EXTRA_DESC(room_data, ed)
            }

            else if (letter == 'O') {
                if (room_data->owner[0] != '\0') {
                    bug("Load_rooms: duplicate owner.", 0);
                    exit(1);
                }

                room_data->owner = fread_string(fp);
            }

            else if (letter == 'V') {
                load_event(fp, &room_data->header);
            }

            else if (letter == 'L') {
                if (!load_lox_class(fp, "room", &room_data->header)) {
                    bug("Load_rooms: vnum %"PRVNUM" has malformed Lox script.", vnum);
                    exit(1);
                }
            }
            else {
                bug("Load_rooms: vnum %"PRVNUM" has invalid option '%c'.", vnum, letter);
                exit(1);
            }
        }

        table_set_vnum(&global_rooms, vnum, OBJ_VAL(room_data));
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;
        assign_area_vnum(vnum);
        pop();
    }

    return;
}
