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
#include <entities/mob_prototype.h>
#include <entities/obj_prototype.h>
#include <entities/object.h>
#include <entities/room.h>
#include <entities/descriptor.h>

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
    global_mob_proto_set(mp);  // Register globally so mload can find it
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
    if (name != NULL) {
        SET_NAME(m, AS_STRING(mock_str(name)));
        m->short_descr = str_dup(name);
    }
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
    global_obj_proto_set(op);  // Register globally so oload can find it
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
    if (name != NULL) {
        SET_NAME(o, AS_STRING(mock_str(name)));
        o->short_descr = str_dup(name);
    }
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

    sword_proto->weapon.weapon_type = WEAPON_SWORD;
    sword_proto->weapon.num_dice = dam_dice;
    sword_proto->weapon.size_dice = dam_size;
    sword_proto->weapon.damage_type = DAM_SLASH;

    Object* sword = mock_obj(name, vnum, sword_proto);

    return sword;
}

Object* mock_shield(const char* name, VNUM vnum, LEVEL level)
{
    ObjPrototype* shield_proto = mock_obj_proto(vnum);
    shield_proto->header.name = AS_STRING(mock_str(name));
    shield_proto->short_descr = str_dup(name);

    shield_proto->level = level;
    shield_proto->condition = 100;
    shield_proto->weight = 5;
    shield_proto->cost = level * 15;
    shield_proto->item_type = ITEM_ARMOR;
    shield_proto->armor.ac_pierce = -10;
    shield_proto->armor.ac_bash = -10;
    shield_proto->armor.ac_slash = -10;
    shield_proto->armor.ac_exotic = -10;

    Object* shield = mock_obj(name, vnum, shield_proto);

    return shield;
}

Object* mock_mat(const char* name, VNUM vnum, int mat_type, int amount, int quality)
{
    ObjPrototype* mat_proto = mock_obj_proto(vnum);
    mat_proto->header.name = AS_STRING(mock_str(name));
    mat_proto->short_descr = str_dup(name);

    mat_proto->level = 1;
    mat_proto->condition = 100;
    mat_proto->weight = 1;
    mat_proto->cost = 10 * amount;
    mat_proto->item_type = ITEM_MAT;

    mat_proto->craft_mat.mat_type = mat_type;
    mat_proto->craft_mat.amount = amount;
    mat_proto->craft_mat.quality = quality;

    Object* mat = mock_obj(name, vnum, mat_proto);

    return mat;
}

Object* mock_workstation(const char* name, VNUM vnum, int station_flags, int bonus)
{
    ObjPrototype* ws_proto = mock_obj_proto(vnum);
    ws_proto->header.name = AS_STRING(mock_str(name));
    ws_proto->short_descr = str_dup(name);

    ws_proto->level = 1;
    ws_proto->condition = 100;
    ws_proto->weight = 100;  // Heavy furniture
    ws_proto->cost = 1000;
    ws_proto->item_type = ITEM_WORKSTATION;

    ws_proto->workstation.station_flags = station_flags;
    ws_proto->workstation.bonus = bonus;

    Object* ws = mock_obj(name, vnum, ws_proto);

    return ws;
}

#include <craft/recipe.h>

Recipe* mock_recipe(const char* name, VNUM vnum)
{
    Recipe* recipe = new_recipe();
    recipe->header.vnum = vnum;
    recipe->header.name = AS_STRING(mock_str(name));
    
    // Add to global table
    add_recipe(recipe);
    
    // Track for cleanup
    write_value_array(mocks(), OBJ_VAL(recipe));
    
    return recipe;
}

void mock_skill(Mobile* ch, SKNUM sn, int value)
{
    if (ch == NULL || sn < 0) return;

    if (IS_NPC(ch)) {
        // For NPCs, set appropriate atk_flags for defensive skills
        if (sn == gsn_parry) {
            SET_BIT(ch->atk_flags, ATK_PARRY);
        } else if (sn == gsn_dodge) {
            SET_BIT(ch->atk_flags, ATK_DODGE);
        }
        // For other skills, NPCs calculate based on level (can't override)
    } else {
        // For PCs, set learned value directly
        if (ch->pcdata != NULL && ch->pcdata->learned != NULL) {
            ch->pcdata->learned[sn] = (int16_t)value;
        }
    }
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
        rd1->exit_data[dir]->to_vnum = VNUM_FIELD(rd2);
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
    m->comm_flags = 0;  // Clear all comm flags for clean test state
    m->pcdata = new_player_data();
    m->pcdata->ch = m;
    m->pcdata->bamfin = str_dup("");
    m->pcdata->bamfout = str_dup("");
    m->desc = mock_descriptor();
    m->desc->character = m;
    REMOVE_BIT(m->act_flags, PLR_COLOUR);
    write_value_array(mocks(), OBJ_VAL(m->pcdata));
    return m;
}

Mobile* mock_imm(const char* name)
{
    Mobile* m = mock_player(name);
    m->level = MAX_LEVEL;
    m->trust = MAX_LEVEL;
    m->pcdata->security = 9;  // Full access
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

void mock_connect_player_descriptor(Mobile* player)
{
    if (player == NULL)
        return;

    Descriptor* desc = player->desc ? player->desc : mock_descriptor();
    desc->next = descriptor_list;
    desc->character = player;
    descriptor_list = desc;
    desc->connected = CON_PLAYING;
}

void mock_disconnect_player_descriptor(Mobile* player)
{
    if (player == NULL || player->desc == NULL)
        return;

    Descriptor* target = player->desc;
    Descriptor** link = &descriptor_list;
    while (*link != NULL) {
        if (*link == target) {
            *link = target->next;
            target->next = NULL;
            break;
        }
        link = &(*link)->next;
    }
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
        else if (IS_MOB_PROTO(val)) {
            MobPrototype* mp = AS_MOB_PROTO(val);
            // Remove from global registry
            global_mob_proto_remove(VNUM_FIELD(mp));
        }
        else if (IS_OBJ_PROTO(val)) {
            ObjPrototype* op = AS_OBJ_PROTO(val);
            // Remove from global registry
            global_obj_proto_remove(VNUM_FIELD(op));
        }
        // All other types (Areas, Prototypes, etc.) are managed by GC
    }
}
