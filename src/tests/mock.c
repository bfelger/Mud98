////////////////////////////////////////////////////////////////////////////////
// mock.c
////////////////////////////////////////////////////////////////////////////////

#include "mock.h"

#include <db.h>
#include <handler.h>
#include <recycle.h>

#include <entities/area.h>
#include <entities/faction.h>
#include <entities/mobile.h>
#include <entities/obj_prototype.h>
#include <entities/object.h>
#include <entities/room.h>

// This is marked by Lox's GC
ValueArray* mocks_ = NULL;

ValueArray* mocks()
{
    if (mocks_ == NULL) {
        mocks_ = new_obj_array();
        init_value_array(mocks_);
    }

    return mocks_;
}

Value mock_str(const char* str)
{
    Value v = OBJ_VAL(copy_string(str, (int)strlen(str)));
    write_value_array(mocks(), v);
    return v;
}

AreaData* mock_area_data()
{
    AreaData* ad = new_area_data();
    write_value_array(mocks(), OBJ_VAL(ad));

    return ad;
}

RoomData* mock_room_data(VNUM vnum, AreaData* ad)
{
    if (ad == NULL)
        ad = mock_area_data();

    RoomData* rd = new_room_data();
    write_value_array(mocks(), OBJ_VAL(rd));
    rd->area_data = ad;
    VNUM_FIELD(rd) = vnum;  // Use the vnum parameter instead of hardcoding
    global_room_set(rd);

    return rd;
}

Area* mock_area(AreaData* ad)
{
    if (ad == NULL)
        ad = mock_area_data();

    Area* a;
    if (ad->instances.count == 0 || ad->inst_type == AREA_INST_MULTI) {
        a = create_area_instance(ad, true);
        list_push(&ad->instances, OBJ_VAL(a));
    } else {
        a = AS_AREA(list_first(&ad->instances));
    }
    write_value_array(mocks(), OBJ_VAL(a));

    return a;
}

MobPrototype* mock_mob_proto(VNUM vnum)
{
    MobPrototype* mp = new_mob_prototype();
    mp->header.vnum = vnum;
    mp->sex = SEX_MALE;
    mp->level = 1;
    mp->hit[0] = 1;
    mp->hit[1] = 1;
    mp->hit[2] = 1;
    mp->hitroll = 1;
    write_value_array(mocks(), OBJ_VAL(mp));

    return mp;
}

Mobile* mock_mob(const char* name, VNUM vnum, MobPrototype* mp)
{
    if (mp == NULL) {
        mp = mock_mob_proto(vnum);
        mp->header.name = AS_STRING(mock_str(name));
        mp->short_descr = str_dup(name);
    }

    Mobile* m = create_mobile(mp);
    m->header.name = AS_STRING(mock_str(name));
    m->position = POS_STANDING;
    write_value_array(mocks(), OBJ_VAL(m));

    return m;
}

ObjPrototype* mock_obj_proto(VNUM vnum)
{
    ObjPrototype* op = new_object_prototype();
    op->header.vnum = vnum;
    op->level = 1;
    op->weight = 1;
    op->condition = 100;
    op->material = &str_empty[0];
    write_value_array(mocks(), OBJ_VAL(op));
    return op;
}

Object* mock_obj(const char* name, VNUM vnum, ObjPrototype* op)
{
    if (op == NULL) {
        op = mock_obj_proto(vnum);
        op->header.name = AS_STRING(mock_str(name));
        op->short_descr = str_dup(name);
    }

    Object* o = create_object(op, 0);
    o->header.name = AS_STRING(mock_str(name));
    write_value_array(mocks(), OBJ_VAL(o));
    return o;
}

Object* mock_sword(const char* name, VNUM vnum, LEVEL level, int dam_dice, int dam_size)
{
    ObjPrototype* sword_proto = mock_obj_proto(vnum);
    sword_proto->header.name = AS_STRING(mock_str(name));
    sword_proto->short_descr = str_dup(name);

    sword_proto->level = level;
    sword_proto->condition = 100;
    sword_proto->weight = 3;
    sword_proto->cost = level * 10;
    sword_proto->item_type = ITEM_WEAPON;

    sword_proto->value[0] = WEAPON_SWORD;
    sword_proto->value[1] = dam_dice;
    sword_proto->value[2] = dam_size;
    sword_proto->value[3] = DAM_SLASH;

    Object* sword = mock_obj(name, vnum, sword_proto);

    return sword;
}

Room* mock_room(VNUM vnum, RoomData* rd, Area* a)
{
    if (a == NULL) {
        AreaData* ad = NULL;

        if (rd != NULL && rd->area_data != NULL)
            ad = rd->area_data;
        else
            ad = mock_area_data();

        a = mock_area(ad);
    }

    if (rd == NULL) {
        rd = mock_room_data(vnum, a->data);
    }

    // Mock Room
    Room* r = new_room(rd, a);
    write_value_array(mocks(), OBJ_VAL(r));

    return r;
}

void mock_room_data_connection(RoomData* rd1, RoomData* rd2, Direction dir, bool bidirectional)
{
    if (rd1 != NULL && rd2 != NULL) {
        rd1->exit_data[dir] = new_room_exit_data();
        rd1->exit_data[dir]->to_room = rd2;
    }

    if (bidirectional)
        mock_room_data_connection(rd2, rd1, dir_list[dir].rev_dir, false);
}

void mock_room_connection(Room* r1, Room* r2, Direction dir, bool bidirectional)
{
    if (r1 != NULL && r2 != NULL) {
        if (r1->data->exit_data[dir] == NULL)
            mock_room_data_connection(r1->data, r2->data, dir, false);
        r1->exit[dir] = new_room_exit(r1->data->exit_data[dir], r1);
    }

    if (bidirectional)
        mock_room_connection(r2, r1, dir_list[dir].rev_dir, false);
}

Descriptor* mock_descriptor()
{
    Descriptor* d = new_descriptor();
    return d;
}

Mobile* mock_player(const char* name)
{
    Mobile* m = mock_mob(name, 0, NULL);
    m->act_flags = 0;
    m->pcdata = new_player_data();
    m->pcdata->ch = m;
    m->desc = mock_descriptor();
    m->desc->character = m;
    REMOVE_BIT(m->act_flags, PLR_COLOUR);
    write_value_array(mocks(), OBJ_VAL(m->pcdata));
    return m;
}

Faction* mock_faction(const char* name, VNUM vnum)
{
    Faction* faction = faction_create(vnum);
    if (faction != NULL) {
        faction->header.name = AS_STRING(mock_str(name));
        faction->default_standing = 0;
    }
    return faction;
}

void mock_player_reputation(Mobile* ch, VNUM faction_vnum, int value)
{
    if (ch == NULL || IS_NPC(ch) || faction_vnum == 0)
        return;
    
    faction_set(ch->pcdata, faction_vnum, value);
}

void cleanup_mocks()
{
    if (mocks_ == NULL)
        return;

    // Only remove entities from global lists - GC will free memory
    while (mocks_->count > 0) {
        Value val = mocks_->values[--mocks_->count];
        
        if (IS_MOBILE(val)) {
            Mobile* mob = AS_MOBILE(val);
            if (IS_VALID(mob)) {
                // Just remove from room - don't extract (too aggressive)
                if (mob->in_room != NULL)
                    mob_from_room(mob);
                // Remove from global mob_list
                if (mob->mob_list_node != NULL) {
                    list_remove_node(&mob_list, mob->mob_list_node);
                    mob->mob_list_node = NULL;
                }
            }
        }
        else if (IS_OBJECT(val)) {
            Object* obj = AS_OBJECT(val);
            if (IS_VALID(obj)) {
                // Just remove from wherever it is - don't extract
                if (obj->in_room != NULL)
                    obj_from_room(obj);
                else if (obj->carried_by != NULL)
                    obj_from_char(obj);
                else if (obj->in_obj != NULL)
                    obj_from_obj(obj);
                // Remove from global obj_list
                if (obj->obj_list_node != NULL) {
                    list_remove_node(&obj_list, obj->obj_list_node);
                    obj->obj_list_node = NULL;
                }
            }
        }
        else if (IS_ROOM_DATA(val)) {
            RoomData* rd = AS_ROOM_DATA(val);
            // Remove from global registry so it doesn't conflict with future tests
            global_room_remove(VNUM_FIELD(rd));
        }
        // All other types (Areas, Prototypes, etc.) are managed by GC
    }
}
