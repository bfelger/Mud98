////////////////////////////////////////////////////////////////////////////////
// handler.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__HANDLER_H
#define MUD98__HANDLER_H

#include "entities/mobile.h"
#include "entities/object.h"
#include "entities/room.h"

#include "data/damage.h"
#include "data/item.h"

typedef enum money_type_t {
    MONEY_TYPE_COPPER = 0,
    MONEY_TYPE_SILVER = 1,
    MONEY_TYPE_GOLD = 2,
} MoneyType;

const char* money_type_token(MoneyType type);
const char* money_type_label(MoneyType type, int amount);
int money_type_value(MoneyType type);
int16_t* money_type_ptr(Mobile* ch, MoneyType type);
bool parse_money_type(const char* word, MoneyType* out);

void all_colour(Mobile* ch, char* argument);
int count_users(Object* obj);
void deduct_cost(Mobile* ch, long cost);
void format_money_string(char* buf, size_t len, int gold, int silver, int copper, bool compact);
ResistType check_immune(Mobile* ch, DamageType dam_type);
int material_lookup(const char* name);
WeaponType weapon_lookup(const char* name);
int attack_lookup(const char* name);
int wiznet_lookup(const char* name);
int16_t class_lookup(const char* name);
bool is_clan(Mobile* ch);
bool is_same_clan(Mobile* ch, Mobile* victim);
int get_skill(Mobile* ch, SKNUM sn);
SKNUM get_weapon_sn(Mobile* ch);
int get_weapon_skill(Mobile* ch, SKNUM sn);
int get_age(Mobile* ch);
void reset_char(Mobile* ch);
LEVEL get_trust(Mobile* ch);
int get_curr_stat(Mobile* ch, Stat stat);
int get_max_train(Mobile* ch, Stat stat);
int can_carry_n(Mobile* ch);
int can_carry_w(Mobile* ch);
bool is_name(char* str, char* namelist);
bool is_exact_name(char* str, char* namelist);
void transfer_mob(Mobile* ch, Room* room);
void mob_from_room(Mobile* ch);
void mob_to_room(Mobile* ch, Room* pRoomIndex);
void obj_to_char(Object* obj, Mobile* ch);
void obj_from_char(Object* obj);
int apply_ac(Object* obj, int iWear, int type);
Object* get_eq_char(Mobile* ch, WearLocation iWear);
void equip_char(Mobile* ch, Object* obj, WearLocation iWear);
void unequip_char(Mobile* ch, Object* obj);
int count_obj_list(ObjPrototype* obj, List* list);
void obj_from_room(Object* obj);
void obj_to_room(Object* obj, Room* pRoomIndex);
void obj_to_obj(Object* obj, Object* obj_to);
void obj_from_obj(Object* obj);
void extract_obj(Object* obj);
void extract_char(Mobile* ch, bool fPull);
Mobile* get_mob_room(Mobile* ch, char* argument);
Mobile* get_mob_world(Mobile* ch, char* argument);
Object* get_obj_type(ObjPrototype* pObjIndexData);
Object* get_obj_list(Mobile* ch, char* argument, List* list);
Object* get_obj_carry(Mobile* ch, char* argument);
Object* get_obj_wear(Mobile* ch, char* argument);
Object* get_obj_here(Mobile* ch, char* argument);
Object* get_obj_world(Mobile* ch, char* argument);
Object* create_money(int16_t gold, int16_t silver, int16_t copper);
int get_obj_number(Object* obj);
int get_obj_weight(Object* obj);
int get_true_weight(Object* obj);
bool room_is_dark(Room* pRoomIndex);
bool is_room_owner(Mobile* ch, Room* room);
bool room_is_private(Room* pRoomIndex);
bool can_see(Mobile* ch, Mobile* victim);
bool can_see_obj(Mobile* ch, Object* obj);
bool can_see_room(Mobile* ch, RoomData* pRoomIndex);
bool can_drop_obj(Mobile* ch, Object* obj);
char* extra_bit_name(int extra_flags);
char* wear_bit_name(int wear_flags);
char* act_bit_name(int act_flags);
char* off_bit_name(int atk_flags);
char* imm_bit_name(int imm_flags);
char* form_bit_name(int form_flags);
char* part_bit_name(int part_flags);
char* weapon_bit_name(int weapon_flags);
char* comm_bit_name(int comm_flags);
char* cont_bit_name(int cont_flags);
bool emptystring(const char*);
char* itos(int);
int get_vnum_mob_name_area(char*, AreaData*);
int get_vnum_obj_name_area(char*, AreaData*);
int get_points(int race, int args);

#endif // !MUD98__HANDLER_H
