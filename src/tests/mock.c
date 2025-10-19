////////////////////////////////////////////////////////////////////////////////
// mock.c
////////////////////////////////////////////////////////////////////////////////

#include "mock.h"

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
