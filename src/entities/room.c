////////////////////////////////////////////////////////////////////////////////
// room.c
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

#include "room.h"

#include "comm.h"
#include "db.h"
#include "handler.h"

#include "room_exit.h"
#include "extra_desc.h"
#include "reset.h"

#include "lox/lox.h"

int room_count;
int room_perm_count;
Room* room_free;

int room_data_count;
int room_data_perm_count;
RoomData* room_data_free;
RoomData* room_data_hash_table[MAX_KEY_HASH];

VNUM top_vnum_room;

Room* new_room(RoomData* room_data, Area* area)
{
    LIST_ALLOC_PERM(room, Room);

    room->data = room_data;
    room->next_instance = room_data->instances;
    room_data->instances = room;

    room->area = area;
    room->vnum = room_data->vnum;

    int hash = room->vnum % AREA_ROOM_VNUM_HASH_SIZE;

    ORDERED_INSERT(Room, room, area->rooms[hash], vnum);

    return room;
}

void free_room(Room* room)
{
    if (room == room->data->instances)
        room->data->instances = room->next_instance;

    int hash = room->vnum % AREA_ROOM_VNUM_HASH_SIZE;
    Area* area = room->area;

    UNORDERED_REMOVE(Room, room, area->rooms[hash], vnum, room->vnum);

    LIST_FREE(room);
}

RoomData* new_room_data()
{
    LIST_ALLOC_PERM(room_data, RoomData);

    room_data->name = &str_empty[0];
    room_data->description = &str_empty[0];
    room_data->owner = &str_empty[0];
    room_data->heal_rate = 100;
    room_data->mana_rate = 100;

    return room_data;
}

void free_room_data(RoomData* room_data)
{
    ExtraDesc* extra;
    Reset* reset;
    int i;

    free_string(room_data->name);
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

    ORDERED_GET(RoomData, room_data, room_data_hash_table[vnum % MAX_KEY_HASH], vnum, vnum);

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
            bug("get_room: bad vnum %"PRVNUM".", vnum);
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
        search_area = room_data->area_data->instances;
    }
    else {
        return NULL;
    }

    int hash = vnum % AREA_ROOM_VNUM_HASH_SIZE;
    Room* room = NULL;
    ORDERED_GET(Room, room, search_area->rooms[hash], vnum, vnum);

    if (fBootDb && !room) {
        bug("get_room: bad vnum %"PRVNUM".", vnum);
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
    printf_log("Creating new instance '%s' for %s.", room_data->area_data->name,
        ch->name);
    area = create_area_instance(room_data->area_data, true);
    INIT_BUF(buf, MSL);
    addf_buf(buf, "%s %s", ch->name, area->owner_list);
    RESTRING(area->owner_list, BUF(buf));
    reset_area(area);
    free_buf(buf);
    return get_room(area, vnum);
}

////////////////////////////////////////////////////////////////////////////////
// Lox representation
////////////////////////////////////////////////////////////////////////////////

static ObjClass* room_class = NULL;

void init_room_class()
{
    char* source =
        "class Room { "
        "   name() { return marshal(this._name); }"
        "   vnum() { return marshal(this._vnum); }"
        "}";

    InterpretResult result = interpret_code(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);

    room_class = find_class("Room");
}

Value create_room_value(Room* room)
{
    if (!room || !room_class || !room->data)
        return NIL_VAL;

    ObjInstance* inst = new_instance(room_class);
    push(OBJ_VAL(inst));

    SET_NATIVE_FIELD(inst, room->data->name, name, STR);
    SET_NATIVE_FIELD(inst, room->data->vnum, vnum, I32);

    pop(); // instance

    return OBJ_VAL(inst);
}

Value get_room_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        printf("get_room() takes 1 argument; %d given.", arg_count);
        return NIL_VAL;
    }

    if (IS_RAW_PTR(args[0])) {
        ObjRawPtr* ptr = AS_RAW_PTR(args[0]);
        if (ptr->type == RAW_OBJ) {
            Value room =  create_room_value((Room*)ptr->addr);
            return room;
        }
    }

    printf("get_room(): argument is incorrect type:");
    print_value(args[0]);
    return NIL_VAL;
}
