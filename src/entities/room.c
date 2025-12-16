////////////////////////////////////////////////////////////////////////////////
// entities/room.c
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

#include "room.h"

#include "area.h"

#include "room_exit.h"
#include "event.h"
#include "extra_desc.h"
#include "reset.h"

#include <lox/lox.h>
#include <lox/ordered_table.h>
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

static OrderedTable global_rooms;

VNUM top_vnum_room;

void init_global_rooms(void)
{
    ordered_table_init(&global_rooms);
}

void free_global_rooms(void)
{
    ordered_table_free(&global_rooms);
}

RoomData* global_room_get(VNUM vnum)
{
    Value val;
    if (ordered_table_get_vnum(&global_rooms, vnum, &val) && IS_ROOM_DATA(val))
        return AS_ROOM_DATA(val);
    return NULL;
}

bool global_room_set(RoomData* room_data)
{
    if (room_data == NULL)
        return false;
    return ordered_table_set_vnum(&global_rooms, VNUM_FIELD(room_data), OBJ_VAL(room_data));
}

bool global_room_remove(VNUM vnum)
{
    return ordered_table_delete_vnum(&global_rooms, (int32_t)vnum);
}

int global_room_count(void)
{
    return ordered_table_count(&global_rooms);
}

GlobalRoomIter make_global_room_iter(void)
{
    GlobalRoomIter iter = { ordered_table_iter(&global_rooms) };
    return iter;
}

RoomData* global_room_iter_next(GlobalRoomIter* iter)
{
    if (iter == NULL)
        return NULL;

    Value value;
    int32_t key;
    while (ordered_table_iter_next(&iter->iter, &key, &value)) {
        if (IS_ROOM_DATA(value))
            return AS_ROOM_DATA(value);
    }

    return NULL;
}

OrderedTable snapshot_global_rooms(void)
{
    return global_rooms;
}

void restore_global_rooms(OrderedTable snapshot)
{
    global_rooms = snapshot;
}

void mark_global_rooms(void)
{
    mark_ordered_table(&global_rooms);
}

Room* new_room(RoomData* room_data, Area* area)
{
    LIST_ALLOC_PERM(room, Room);

    gc_protect(OBJ_VAL(room));

    init_header(&room->header, OBJ_ROOM);

    init_list(&room->mobiles);
    SET_LOX_FIELD(&room->header, &room->mobiles, mobiles);

    init_list(&room->objects);
    SET_LOX_FIELD(&room->header, &room->objects, objects);

    init_list(&room->inbound_exits);

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

    return room;
}

void free_room(Room* room)
{
    Area* area = room->area;

    // Clean up outbound exits
    for (Direction dir = 0; dir < DIR_MAX; dir++) {
        if (room->exit[dir]) {
            free_room_exit(room->exit[dir]);
            room->exit[dir] = NULL;
        }
    }

    // Clean up inbound exit references (unless doing bulk area teardown)
    if (!area->teardown_in_progress) {
        // Use the inbound_exits list to find ALL exits pointing to this room,
        // regardless of which area they're in (handles cross-area exits!)
        Node* node = room->inbound_exits.front;
        while (node != NULL) {
            RoomExit* inbound_exit = (RoomExit*)AS_OBJ(node->value);
            Node* next_node = node->next;  // Save next before we free the exit
            
            // Find which direction this exit is in the source room and free it
            Room* source_room = inbound_exit->from_room;
            for (Direction dir = 0; dir < DIR_MAX; dir++) {
                if (source_room->exit[dir] == inbound_exit) {
                    free_room_exit(source_room->exit[dir]);
                    source_room->exit[dir] = NULL;
                    break;
                }
            }
            
            node = next_node;
        }
    }

    free_list(&room->inbound_exits);
    list_remove_value(&room->data->instances, OBJ_VAL(room));
    table_delete_vnum(&area->rooms, VNUM_FIELD(room));

    LIST_FREE(room);
}

DayCyclePeriod* room_daycycle_period_add(RoomData* room, const char* name, int start_hour, int end_hour)
{
    if (!room)
        return NULL;

    DayCyclePeriod* period = daycycle_period_new(name ? name : "", start_hour, end_hour);
    daycycle_period_append(&room->periods, period);
    return period;
}

DayCyclePeriod* area_daycycle_period_add(AreaData* area, const char* name, int start_hour, int end_hour)
{
    if (!area)
        return NULL;

    DayCyclePeriod* period = daycycle_period_new(name ? name : "", start_hour, end_hour);
    daycycle_period_append(&area->periods, period);
    return period;
}

DayCyclePeriod* room_daycycle_period_find(RoomData* room, const char* name)
{
    if (!room)
        return NULL;
    return daycycle_period_find(room->periods, name);
}

bool room_daycycle_period_remove(RoomData* room, const char* name)
{
    if (!room)
        return false;
    return daycycle_period_remove(&room->periods, name);
}

void room_daycycle_period_clear(RoomData* room)
{
    if (!room)
        return;
    daycycle_period_clear(&room->periods);
}

DayCyclePeriod* area_daycycle_period_find(AreaData* area, const char* name)
{
    if (!area)
        return NULL;
    return daycycle_period_find(area->periods, name);
}

bool area_daycycle_period_remove(AreaData* area, const char* name)
{
    if (!area)
        return false;
    return daycycle_period_remove(&area->periods, name);
}

void area_daycycle_period_clear(AreaData* area)
{
    if (!area)
        return;
    daycycle_period_clear(&area->periods);
}

DayCyclePeriod* room_daycycle_period_clone(const DayCyclePeriod* head)
{
    return daycycle_period_clone_list(head);
}

const char* room_description_for_hour(const RoomData* room, int hour)
{
    if (!room)
        return &str_empty[0];

    hour = daycycle_normalize_hour(hour);

    for (DayCyclePeriod* period = room->periods; period != NULL; period = period->next) {
        if (period->description && period->description[0] != '\0' && daycycle_period_contains_hour(period, hour))
            return period->description;
    }

    return room->description ? room->description : &str_empty[0];
}

bool room_suppresses_daycycle_messages(const RoomData* room)
{
    if (!room)
        return false;

    if (room->suppress_daycycle_messages)
        return true;

    if (room->area_data && room->area_data->suppress_daycycle_messages)
        return true;

    return false;
}

static void send_period_message_to_room(Room* room, const char* message)
{
    Mobile* ch;

    if (room == NULL || message == NULL || message[0] == '\0')
        return;

    FOR_EACH_ROOM_MOB(ch, room) {
        if (IS_NPC(ch) || ch->desc == NULL)
            continue;
        if (!IS_AWAKE(ch))
            continue;
        send_to_char(message, ch);
    }
}

static void send_period_message_to_instances(RoomData* room_data, const char* message)
{
    Room* room;

    if (room_data == NULL || message == NULL || message[0] == '\0')
        return;

    FOR_EACH_ROOM_INST(room, room_data) {
        if (room != NULL)
            send_period_message_to_room(room, message);
    }
}

static void send_period_message_to_area(AreaData* area_data, const char* message)
{
    if (area_data == NULL || message == NULL || message[0] == '\0')
        return;

    Area* area_inst;
    FOR_EACH_AREA_INST(area_inst, area_data) {
        if (area_inst == NULL)
            continue;
        Room* room;
        FOR_EACH_AREA_ROOM(room, area_inst) {
            if (room == NULL || room->data == NULL)
                continue;
            if (room->data->suppress_daycycle_messages)
                continue;
            send_period_message_to_room(room, message);
        }
    }
}

bool room_has_period_message_transition(const RoomData* room, int old_hour, int new_hour)
{
    if (room == NULL || room->periods == NULL)
        return false;

    old_hour = daycycle_normalize_hour(old_hour);
    new_hour = daycycle_normalize_hour(new_hour);

    for (DayCyclePeriod* period = room->periods; period != NULL; period = period->next) {
        bool was_active = daycycle_period_contains_hour(period, old_hour);
        bool now_active = daycycle_period_contains_hour(period, new_hour);

        if (now_active && !was_active && period->enter_message && period->enter_message[0] != '\0')
            return true;
        if (was_active && !now_active && period->exit_message && period->exit_message[0] != '\0')
            return true;
    }

    return false;
}

void broadcast_room_period_messages(int old_hour, int new_hour)
{
    RoomData* room_data;

    if (old_hour == new_hour)
        return;

    old_hour = daycycle_normalize_hour(old_hour);
    new_hour = daycycle_normalize_hour(new_hour);

    FOR_EACH_GLOBAL_ROOM(room_data) {
        if (room_data->periods == NULL)
            continue;

        for (DayCyclePeriod* period = room_data->periods; period != NULL; period = period->next) {
            bool was_active = daycycle_period_contains_hour(period, old_hour);
            bool now_active = daycycle_period_contains_hour(period, new_hour);

            // Period ending: fire TRIG_PRDSTOP BEFORE exit message
            if (was_active && !now_active) {
                Room* room_inst;
                FOR_EACH_ROOM_INST(room_inst, room_data) {
                    raise_prdstop_event((Entity*)room_inst, period->name);
                }
                
                if (period->exit_message && period->exit_message[0] != '\0')
                    send_period_message_to_instances(room_data, period->exit_message);
            }

            // Period starting: fire TRIG_PRDSTART AFTER enter message
            if (now_active && !was_active) {
                if (period->enter_message && period->enter_message[0] != '\0')
                    send_period_message_to_instances(room_data, period->enter_message);
                
                Room* room_inst;
                FOR_EACH_ROOM_INST(room_inst, room_data) {
                    raise_prdstart_event((Entity*)room_inst, period->name);
                }
            }
        }
    }
}

void broadcast_area_period_messages(int old_hour, int new_hour)
{
    AreaData* area_data;

    if (old_hour == new_hour)
        return;

    old_hour = daycycle_normalize_hour(old_hour);
    new_hour = daycycle_normalize_hour(new_hour);

    FOR_EACH_AREA(area_data) {
        if (area_data->periods == NULL)
            continue;

        for (DayCyclePeriod* period = area_data->periods; period != NULL; period = period->next) {
            bool was_active = daycycle_period_contains_hour(period, old_hour);
            bool now_active = daycycle_period_contains_hour(period, new_hour);

            // Period ending: fire TRIG_PRDSTOP BEFORE exit message
            if (was_active && !now_active) {
                Area* area_inst;
                FOR_EACH_AREA_INST(area_inst, area_data) {
                    raise_prdstop_event((Entity*)area_inst, period->name);
                }
                
                if (period->exit_message && period->exit_message[0] != '\0')
                    send_period_message_to_area(area_data, period->exit_message);
            }

            // Period starting: fire TRIG_PRDSTART AFTER enter message
            if (now_active && !was_active) {
                if (period->enter_message && period->enter_message[0] != '\0')
                    send_period_message_to_area(area_data, period->enter_message);
                
                Area* area_inst;
                FOR_EACH_AREA_INST(area_inst, area_data) {
                    raise_prdstart_event((Entity*)area_inst, period->name);
                }
            }
        }
    }
}

RoomData* new_room_data()
{
    LIST_ALLOC_PERM(room_data, RoomData);

    gc_protect(OBJ_VAL(room_data));

    init_header(&room_data->header, OBJ_ROOM_DATA);

    init_list(&room_data->instances);
    SET_LOX_FIELD(&room_data->header, &room_data->instances, instances);

    SET_NAME(room_data, lox_empty_string);
    room_data->description = &str_empty[0];
    room_data->owner = &str_empty[0];
    room_data->heal_rate = 100;
    room_data->mana_rate = 100;
    room_data->periods = NULL;
    room_data->suppress_daycycle_messages = false;

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

    room_daycycle_period_clear(room_data);

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
    RoomData* room_data = global_room_get(vnum);

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
            else if (letter == 'P') {
                const char* name = fread_word(fp);
                int start = fread_number(fp);
                int end = fread_number(fp);
                DayCyclePeriod* period = room_daycycle_period_add(room_data, name, start, end);
                if (period == NULL) {
                    bug("Load_rooms: failed to add time period '%s' to room %"PRVNUM".", name ? name : "", vnum);
                    exit(1);
                }
                free_string(period->description);
                period->description = fread_string(fp);
            }
            else if (letter == 'B') {
                const char* name = fread_word(fp);
                DayCyclePeriod* period = room_daycycle_period_find(room_data, name);
                if (period == NULL) {
                    bug("Load_rooms: period '%s' not found for enter message in room %"PRVNUM".", name ? name : "", vnum);
                    exit(1);
                }
                free_string(period->enter_message);
                period->enter_message = fread_string(fp);
            }
            else if (letter == 'A') {
                const char* name = fread_word(fp);
                DayCyclePeriod* period = room_daycycle_period_find(room_data, name);
                if (period == NULL) {
                    bug("Load_rooms: period '%s' not found for exit message in room %"PRVNUM".", name ? name : "", vnum);
                    exit(1);
                }
                free_string(period->exit_message);
                period->exit_message = fread_string(fp);
            }
            else if (letter == 'W') {
                room_data->suppress_daycycle_messages = fread_number(fp) != 0;
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

        global_room_set(room_data);
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;
        assign_area_vnum(vnum);
    }

    return;
}
