////////////////////////////////////////////////////////////////////////////////
// mock.h
// 
// Mocking utilities for unit tests. All mocks are tracked by Lox's GC and
// automatically cleaned up between tests.
// 
// CRITICAL: Entities must be placed in rooms before executing commands!
// Commands assume world structure exists (entities reference room->area, 
// room->people, etc.). Always use transfer_mob() after creation.
// 
// Example:
//     Room* room = mock_room(50000, NULL, NULL);
//     Mobile* ch = mock_player("Bob");
//     transfer_mob(ch, room);  // REQUIRED before do_* commands
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TESTS__MOCK_H
#define MUD98__TESTS__MOCK_H

#include <entities/area.h>
#include <entities/obj_prototype.h>

#include <lox/array.h>

// Internal GC-tracked array of all mocks
ValueArray* mocks();

// Clean up all mocks (called after each test)
void cleanup_mocks();

// Create a GC-tracked Lox string
Value mock_str(const char* str);

// AREA MOCKING
// Creates minimal area data/instances for testing
AreaData* mock_area_data();
Area* mock_area(AreaData* ad);

// ROOM MOCKING
// mock_room_data: Creates RoomData (template/prototype layer)
// mock_room: Creates Room instance (runtime layer with exits[], people, etc.)
// 
// For most tests, use mock_room(vnum, NULL, NULL) - it auto-creates parents.
RoomData* mock_room_data(VNUM vnum, AreaData* ad);
Room* mock_room(VNUM vnum, RoomData* rd, Area* a);

// ROOM CONNECTION MOCKING
// CRITICAL: Commands check Room->exit[], not RoomData->exit_data[]!
// 
// mock_room_data_connection: Sets up RoomData->exit_data[] (template layer)
// mock_room_connection: Sets up Room->exit[] (runtime layer) 
//                       This is what move_char() checks!
// 
// For movement tests, ALWAYS use mock_room_connection():
//     Room* r1 = mock_room(50000, NULL, NULL);
//     Room* r2 = mock_room(50001, NULL, NULL);
//     mock_room_connection(r1, r2, DIR_NORTH, true);
void mock_room_data_connection(RoomData* rd1, RoomData* rd2, Direction dir, bool bidirectional);
void mock_room_connection(Room* r1, Room* r2, Direction dir, bool bidirectional);

// MOBILE MOCKING
// mock_mob_proto: Creates MobPrototype (template)
// mock_mob: Creates Mobile instance (NPC)
// mock_player: Creates Mobile with pcdata (player character)
// 
// Mob requires prototype if you need specific stats:
//     MobPrototype* proto = mock_mob_proto(53001);
//     proto->level = 10;
//     Mobile* mob = mock_mob("Guard", 53001, proto);
// 
// Or use NULL for defaults:
//     Mobile* mob = mock_mob("Guard", 53001, NULL);
MobPrototype* mock_mob_proto(VNUM vnum);
Mobile* mock_mob(const char* name, VNUM vnum, MobPrototype* mp);
Mobile* mock_player(const char* name);
Mobile* mock_imm(const char* name);

// OBJECT MOCKING  
// mock_obj_proto: Creates ObjPrototype (template)
// mock_obj: Creates Object instance (generic item)
// mock_sword: Creates weapon with specific damage
// 
// CRITICAL: Object header.name field must contain searchable keywords!
// get_obj_carry() searches this field, so name="sword blade" allows
// both "get sword" and "get blade" to find it.
ObjPrototype* mock_obj_proto(VNUM vnum);
Object* mock_obj(const char* name, VNUM vnum, ObjPrototype* op);
Object* mock_sword(const char* name, VNUM vnum, LEVEL level, int dam_dice, int dam_size);

// DESCRIPTOR MOCKING
// Creates network connection stub for player tests
Descriptor* mock_descriptor();

// FACTION SYSTEM MOCKING
Faction* mock_faction(const char* name, VNUM vnum);
void mock_player_reputation(Mobile* ch, VNUM faction_vnum, int value);

#endif  // !MUD98__TESTS__MOCK_H
