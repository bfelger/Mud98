////////////////////////////////////////////////////////////////////////////////
// handler.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__HANDLER_H
#define MUD98__HANDLER_H

#include "entities/char_data.h"
#include "entities/object_data.h"
#include "entities/room_data.h"

#include "data/damage.h"
#include "data/item.h"

void all_colour(CharData* ch, char* argument);
int count_users(ObjectData* obj);
void deduct_cost(CharData* ch, int cost);
ResistType check_immune(CharData* ch, DamageType dam_type);
int material_lookup(const char* name);
WeaponType weapon_lookup(const char* name);
int attack_lookup(const char* name);
int wiznet_lookup(const char* name);
int16_t class_lookup(const char* name);
bool is_clan(CharData* ch);
bool is_same_clan(CharData* ch, CharData* victim);
bool is_old_mob(CharData* ch);
int get_skill(CharData* ch, SKNUM sn);
SKNUM get_weapon_sn(CharData* ch);
int get_weapon_skill(CharData* ch, SKNUM sn);
int get_age(CharData* ch);
void reset_char(CharData* ch);
LEVEL get_trust(CharData* ch);
int get_curr_stat(CharData* ch, Stat stat);
int get_max_train(CharData* ch, Stat stat);
int can_carry_n(CharData* ch);
int can_carry_w(CharData* ch);
bool is_name(char* str, char* namelist);
bool is_exact_name(char* str, char* namelist);
void char_from_room(CharData* ch);
void char_to_room(CharData* ch, RoomData* pRoomIndex);
void obj_to_char(ObjectData* obj, CharData* ch);
void obj_from_char(ObjectData* obj);
int apply_ac(ObjectData* obj, int iWear, int type);
ObjectData* get_eq_char(CharData* ch, int iWear);
void equip_char(CharData* ch, ObjectData* obj, int16_t iWear);
void unequip_char(CharData* ch, ObjectData* obj);
int count_obj_list(ObjectPrototype* obj, ObjectData* list);
void obj_from_room(ObjectData* obj);
void obj_to_room(ObjectData* obj, RoomData* pRoomIndex);
void obj_to_obj(ObjectData* obj, ObjectData* obj_to);
void obj_from_obj(ObjectData* obj);
void extract_obj(ObjectData* obj);
void extract_char(CharData* ch, bool fPull);
CharData* get_char_room(CharData* ch, char* argument);
CharData* get_char_world(CharData* ch, char* argument);
ObjectData* get_obj_type(ObjectPrototype* pObjIndexData);
ObjectData* get_obj_list(CharData* ch, char* argument, ObjectData* list);
ObjectData* get_obj_carry(CharData* ch, char* argument, CharData* viewer);
ObjectData* get_obj_wear(CharData* ch, char* argument);
ObjectData* get_obj_here(CharData* ch, char* argument);
ObjectData* get_obj_world(CharData* ch, char* argument);
ObjectData* create_money(int16_t gold, int16_t silver);
int get_obj_number(ObjectData* obj);
int get_obj_weight(ObjectData* obj);
int get_true_weight(ObjectData* obj);
bool room_is_dark(RoomData* pRoomIndex);
bool is_room_owner(CharData* ch, RoomData* room);
bool room_is_private(RoomData* pRoomIndex);
bool can_see(CharData* ch, CharData* victim);
bool can_see_obj(CharData* ch, ObjectData* obj);
bool can_see_room(CharData* ch, RoomData* pRoomIndex);
bool can_drop_obj(CharData* ch, ObjectData* obj);
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
