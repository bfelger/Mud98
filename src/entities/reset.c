////////////////////////////////////////////////////////////////////////////////
// entities/reset.c
// Utilities to handle area resets of mobs and items
////////////////////////////////////////////////////////////////////////////////

#include "reset.h"

#include <comm.h>
#include <db.h>

int reset_count;
int reset_perm_count;
Reset* reset_free;

Reset* new_reset()
{
    LIST_ALLOC_PERM(reset, Reset);

    reset->command = 'X';

    return reset;
}

void free_reset(Reset* reset)
{
    LIST_FREE(reset);
}

// Boot and OLC routines

void reset_room(Room* room)
{
    Reset* reset;
    Mobile* mob = NULL;
    Object* obj;
    Mobile* last_mob = NULL;
    Object* last_obj = NULL;
    int iExit;
    int level = 0;
    bool last;

    if (!room)
        return;

    if (!room->data) {
        bug("Reset_room: room->data is NULL for room vnum %"PRVNUM".", 
            VNUM_FIELD(room));
        return;
    }

    last = false;

    for (iExit = 0; iExit < DIR_MAX; iExit++) {
        RoomExit* room_exit;
        if ((room_exit = room->exit[iExit])) {
            room_exit->exit_flags = room_exit->data->exit_reset_flags;
            if ((room_exit->to_room != NULL)
                && ((room_exit = room_exit->to_room->exit[dir_list[iExit].rev_dir]))) {
                  /* nail the other side */
                room_exit->exit_flags = room_exit->data->exit_reset_flags;
            }
        }
    }

    FOR_EACH(reset, room->data->reset_first)
    {
        MobPrototype* mob_proto;
        ObjPrototype* obj_proto;
        ObjPrototype* obj_to_proto;
        Room* target_room;
        char buf[MAX_STRING_LENGTH];
        int count, limit = 0;

        // Defensive check - reset should never be NULL in a well-formed list
        if (!reset) {
            bugf("Reset_room: reset is NULL in room %"PRVNUM" reset list!", 
                VNUM_FIELD(room));
            break;
        }

        // Defensive check - detect if we're reading garbage memory
        if (reset->command < ' ' || reset->command > '~') {
            bugf("Reset_room: reset->command is garbage (0x%02x) in room %"PRVNUM"!", 
                (unsigned char)reset->command, VNUM_FIELD(room));
            bugf("Reset pointer: %p, next: %p", (void*)reset, (void*)reset->next);
            break;
        }

        switch (reset->command) {
        default:
            bug("Reset_room: bad command %c.", reset->command);
            break;

        case 'M':
        {
            if ((mob_proto = get_mob_prototype(reset->arg1)) == NULL) {
                bug("Reset_room: 'M': bad vnum %"PRVNUM".", reset->arg1);
                last = false;
                continue;
            }

            if ((target_room = get_room(room->area, reset->arg3)) == NULL) {
                bug("Reset_area: 'R': bad vnum %"PRVNUM".", reset->arg3);
                last = false;
                continue;
            }

            if (reset->arg2 == -1)
                limit = 999;    // No limit
            else
                limit = reset->arg2;

            if (mob_proto->count >= limit) {
                last = false;
                break;
            }

            /* */

            count = 0;
            Mobile* temp_mob;
            FOR_EACH_ROOM_MOB(temp_mob, target_room)
                if (temp_mob->prototype == mob_proto) {
                    count++;
                    if (count >= reset->arg4) {
                        last = false;
                        break;
                    }
                }

            if (count >= reset->arg4)
                break;

            /* */

            mob = create_mobile(mob_proto);

            // Some more hard coding.
            if (room_is_dark(room))
                SET_BIT(mob->affect_flags, AFF_INFRARED);

            // Pet shop mobiles get ACT_PET set.
            {
                RoomData* petshop_inv;
                if ((petshop_inv = get_room_data(VNUM_FIELD(room->data) - 1)) != NULL
                    && IS_SET(petshop_inv->room_flags, ROOM_PET_SHOP))
                    SET_BIT(mob->act_flags, ACT_PET);
            }

            mob_to_room(mob, room);
            last_mob = mob;
            level = URANGE(0, mob->level - 2, LEVEL_HERO - 1);
            last = true;
            break;
        }

        case 'O':
            if (!(obj_proto = get_object_prototype(reset->arg1))) {
                bug("Reset_room: 'O' 1 : bad vnum %"PRVNUM"", reset->arg1);
                sprintf(buf, "%"PRVNUM" %d %"PRVNUM" %d", reset->arg1,
                    reset->arg2, reset->arg3, reset->arg4);
                bug(buf, 1);
                continue;
            }

            if (!(target_room = get_room(room->area, reset->arg3))) {
                bug("Reset_room: 'O' 2 : bad vnum %"PRVNUM".", reset->arg3);
                sprintf(buf, "%"PRVNUM" %d %"PRVNUM" %d", reset->arg1,
                    reset->arg2, reset->arg3, reset->arg4);
                bug(buf, 1);
                continue;
            }

            if ((room->area->nplayer > 0 && !room->area->data->always_reset)
                || count_obj_list(obj_proto, &room->objects) > 0
                ) {
                last = false;
                break;
            }

            obj = create_object(obj_proto, (LEVEL)UMIN(number_fuzzy(level),
                LEVEL_HERO - 1));
            obj->cost = 0;
            obj_to_room(obj, room);
            last = true;
            break;

        case 'P':
            if ((obj_proto = get_object_prototype(reset->arg1)) == NULL) {
                bug("Reset_room: 'P': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }
            if ((obj_to_proto = get_object_prototype(reset->arg3)) == NULL) {
                bug("Reset_room: 'P': bad vnum %"PRVNUM".", reset->arg3);
                continue;
            }

            if (reset->arg2 > 50)
                limit = 6;
            else if (reset->arg2 <= 0)
                limit = 999;
            else
                limit = reset->arg2;

            count = 0;
            if ((room->area->nplayer > 0 && !room->area->data->always_reset)
                || (last_obj = get_obj_type(obj_to_proto)) == NULL
                || (last_obj->in_room == NULL && !last)
                || (obj_proto->count >= limit /* && number_range(0,4) != 0 */)
                || (count = count_obj_list(obj_proto, &last_obj->objects))
                > reset->arg4) {
                last = false;
                break;
            }

            /* lastObj->level  -  ROM */
            while (count < reset->arg4) {
                obj = create_object(obj_proto, (LEVEL)number_fuzzy(last_obj->level));
                obj_to_obj(obj, last_obj);
                count++;
                if (obj_proto->count >= limit)
                    break;
            }

            /* fix object lock state! */
            last_obj->value[1] = last_obj->prototype->value[1];
            last = true;
            break;

        case 'G':
        case 'E':
            if ((obj_proto = get_object_prototype(reset->arg1)) == NULL) {
                bug("Reset_room: 'E' or 'G': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }

            if (!last)
                break;

            if (!last_mob) {
                bug("Reset_room: 'E' or 'G': null mob for vnum %"PRVNUM".",
                    reset->arg1);
                last = false;
                break;
            }

            if (last_mob->prototype->pShop != NULL) { // Shopkeeper?
                int olevel = 0;
                obj = create_object(obj_proto, (LEVEL)olevel);
                SET_BIT(obj->extra_flags, ITEM_INVENTORY);
            }
            else {
                if (reset->arg2 > 50)
                    limit = 6;      // Old format
                else if (reset->arg2 == -1 || reset->arg2 == 0)
                    limit = 999;    // No limit
                else
                    limit = reset->arg2;

                if (obj_proto->count < limit || number_range(0, 4) == 0) {
                    obj = create_object(obj_proto,
                        (LEVEL)UMIN(number_fuzzy(level), LEVEL_HERO - 1));
                    // Error message if it is too high
                    if (obj->level > last_mob->level + 3
                        || (obj->item_type == ITEM_WEAPON
                            && reset->command == 'E'
                            && obj->level < last_mob->level - 5 && obj->level < 45))
                        fprintf(stderr,
                            "Err: obj %s [%"PRVNUM"] Lvl %d -- mob %s [%"PRVNUM"] Lvl %d\n",
                            obj->short_descr, VNUM_FIELD(obj->prototype), obj->level,
                            last_mob->short_descr, VNUM_FIELD(last_mob->prototype),
                            last_mob->level);
                }
                else
                    break;
            }

            obj_to_char(obj, last_mob);
            if (reset->command == 'E')
                equip_char(last_mob, obj, reset->arg3);
            last = true;
            break;

        case 'D':
            break;

        case 'R':
            if (!(target_room = get_room(room->area, reset->arg1))) {
                bug("Reset_room: 'R': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }

            {
                RoomExit* room_exit;
                int d0;
                int d1;

                for (d0 = 0; d0 < reset->arg2 - 1; d0++) {
                    d1 = number_range(d0, reset->arg2 - 1);
                    room_exit = target_room->exit[d0];
                    target_room->exit[d0] = target_room->exit[d1];
                    target_room->exit[d1] = room_exit;
                }
            }
            break;
        }
    }

    return;
}

void reset_area(Area* area)
{
    Room* room;

    FOR_EACH_AREA_ROOM(room, area)
        reset_room(room);
}

// Boot routines

static void append_reset(RoomData* room_data, Reset* reset)
{
    Reset* pr;

    if (!room_data)
        return;

    pr = room_data->reset_last;

    if (!pr) {
        room_data->reset_first = reset;
        room_data->reset_last = reset;
    }
    else {
        room_data->reset_last->next = reset;
        room_data->reset_last = reset;
        room_data->reset_last->next = NULL;
    }

    return;
}

void load_resets(FILE* fp)
{
    Reset* reset;
    RoomExitData* room_exit_data;
    RoomData* room_data;
    VNUM vnum = VNUM_NONE;

    if (global_areas.count == 0) {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        char letter;

        if ((letter = fread_letter(fp)) == 'S')
            break;

        if (letter == '*') {
            fread_to_eol(fp);
            continue;
        }

        reset = new_reset();
        reset->command = letter;
        /* if_flag */ fread_number(fp);
        reset->arg1 = fread_vnum(fp);
        reset->arg2 = (int16_t)fread_number(fp);
        reset->arg3 = (letter == 'G' || letter == 'R') ? 0 : fread_vnum(fp);
        reset->arg4 = (letter == 'P' || letter == 'M') ? (int16_t)fread_number(fp) : 0;
        fread_to_eol(fp);

        switch (reset->command) {
        case 'M':
        case 'O':
            vnum = reset->arg3;
            break;

        case 'P':
        case 'G':
        case 'E':
            break;

        case 'D':
            room_data = get_room_data((vnum = reset->arg1));
            if (reset->arg2 < 0
                || reset->arg2 >= DIR_MAX
                || !room_data
                || !(room_exit_data = room_data->exit_data[reset->arg2])
                || !IS_SET(room_exit_data->exit_reset_flags, EX_ISDOOR)) {
                bugf("Load_resets: 'D': exit %d, room %"PRVNUM" not door.", reset->arg2, reset->arg1);
                exit(1);
            }

            switch (reset->arg3) {
            default: bug("Load_resets: 'D': bad 'locks': %d.", reset->arg3); break;
            case 0:
                break;
            case 1:
                SET_BIT(room_exit_data->exit_reset_flags, EX_CLOSED);
                break;
            case 2:
                SET_BIT(room_exit_data->exit_reset_flags, EX_CLOSED | EX_LOCKED);
                break;
            }
            break;

        case 'R':
            vnum = reset->arg1;
            break;
        }

        if (vnum == VNUM_NONE) {
            bugf("load_resets : vnum == VNUM_NONE");
            exit(1);
        }

        if (reset->command != 'D')
            append_reset(get_room_data(vnum), reset);
        else
            free_reset(reset);
    }

    return;
}
