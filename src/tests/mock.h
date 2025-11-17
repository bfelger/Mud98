////////////////////////////////////////////////////////////////////////////////
// mock.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TESTS__MOCK_H
#define MUD98__TESTS__MOCK_H

#include <entities/area.h>
#include <entities/obj_prototype.h>

#include <lox/array.h>

ValueArray* mocks();
Value mock_str(const char* str);

RoomData* mock_room_data(VNUM vnum, AreaData* ad);
Room* mock_room(VNUM vnum, RoomData* rd, Area* a);

void mock_room_data_connection(RoomData* rd1, RoomData* rd2, Direction dir, bool bidirectional);
void mock_room_connection(Room* r1, Room* r2, Direction dir, bool bidirectional);

AreaData* mock_area_data();
Area* mock_area(AreaData* ad);

MobPrototype* mock_mob_proto(VNUM vnum);
Mobile* mock_mob(const char* name, VNUM vnum, MobPrototype* mp);

ObjPrototype* mock_obj_proto(VNUM vnum);
Object* mock_obj(const char* name, VNUM vnum, ObjPrototype* op);
Object* mock_sword(const char* name, VNUM vnum, LEVEL level, int dam_dice, int dam_size);

Descriptor* mock_descriptor();
Mobile* mock_player(const char* name);

#endif // !MUD98__TESTS__MOCK_H
