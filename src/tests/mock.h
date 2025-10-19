////////////////////////////////////////////////////////////////////////////////
// mock.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TESTS__MOCK_H
#define MUD98__TESTS__MOCK_H

#include <entities/area.h>

#include <lox/array.h>

ValueArray* mocks();
Value mock_str(const char* str);

AreaData* mock_area_data();
RoomData* mock_room_data(VNUM vnum, AreaData* ad);
Area* mock_area(AreaData* ad);
Room* mock_room(VNUM vnum, RoomData* rd, Area* a);

#endif // !MUD98__TESTS__MOCK_H
