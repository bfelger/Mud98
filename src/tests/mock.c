////////////////////////////////////////////////////////////////////////////////
// mock.c
////////////////////////////////////////////////////////////////////////////////

#include "mock.h"

#include <db.h>

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
    VNUM_FIELD(rd) = 51000;
    table_set_vnum(&global_rooms, VNUM_FIELD(rd), OBJ_VAL(rd));

    return rd;
}

Area* mock_area(AreaData* ad)
{
    if (ad == NULL)
        ad = mock_area_data();

    Area* a = create_area_instance(ad, false);
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
    Object* sword = mock_obj(name, vnum, NULL);

    sword->level = level;
    sword->condition = 100;
    sword->weight = 3;
    sword->cost = level * 10;
    sword->item_type = ITEM_WEAPON;

    sword->value[0] = WEAPON_SWORD;
    sword->value[1] = dam_dice;
    sword->value[2] = dam_size;
    sword->value[3] = DAM_SLASH;

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
    m->desc = mock_descriptor();
    m->desc->character = m;
    write_value_array(mocks(), OBJ_VAL(m->pcdata));
    return m;
}