////////////////////////////////////////////////////////////////////////////////
// daycycle_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <entities/room.h>

#include <db.h>

TestGroup daycycle_tests;

static RoomData* setup_room(const char* desc)
{
    RoomData* room = mock_room_data(50000, NULL);
    free_string(room->description);
    room->description = str_dup(desc);
    return room;
}

static int test_room_description_defaults_without_periods()
{
    RoomData* room = setup_room("Default description.");

    const char* actual = room_description_for_hour(room, 10);
    ASSERT_STR_EQ("Default description.", actual);

    return 0;
}

static int test_room_description_uses_matching_period()
{
    RoomData* room = setup_room("Default description.");

    RoomTimePeriod* period = room_time_period_add(room, "day", 6, 18);
    free_string(period->description);
    period->description = str_dup("Daytime view.");

    ASSERT_STR_EQ("Default description.", room_description_for_hour(room, 5));
    ASSERT_STR_EQ("Daytime view.", room_description_for_hour(room, 6));
    ASSERT_STR_EQ("Daytime view.", room_description_for_hour(room, 17));

    return 0;
}

static int test_room_description_wraps_across_midnight()
{
    RoomData* room = setup_room("Default description.");

    RoomTimePeriod* period = room_time_period_add(room, "night", 20, 4);
    free_string(period->description);
    period->description = str_dup("Nighttime view.");

    ASSERT_STR_EQ("Nighttime view.", room_description_for_hour(room, 20));
    ASSERT_STR_EQ("Nighttime view.", room_description_for_hour(room, 2));
    ASSERT_STR_EQ("Default description.", room_description_for_hour(room, 10));

    return 0;
}

static int test_room_description_prefers_first_match()
{
    RoomData* room = setup_room("Default description.");

    RoomTimePeriod* first = room_time_period_add(room, "first", 0, 23);
    free_string(first->description);
    first->description = str_dup("First match.");

    RoomTimePeriod* second = room_time_period_add(room, "second", 0, 23);
    free_string(second->description);
    second->description = str_dup("Second match.");

    ASSERT_STR_EQ("First match.", room_description_for_hour(room, 12));

    return 0;
}

static int test_room_description_skips_empty_periods()
{
    RoomData* room = setup_room("Default description.");

    room_time_period_add(room, "blank", 0, 23);

    RoomTimePeriod* filled = room_time_period_add(room, "filled", 0, 23);
    free_string(filled->description);
    filled->description = str_dup("Visible description.");

    ASSERT_STR_EQ("Visible description.", room_description_for_hour(room, 5));

    return 0;
}

void register_daycycle_tests()
{
    init_test_group(&daycycle_tests, "Daycycle");
    register_test(&daycycle_tests, "Defaults Without Periods", test_room_description_defaults_without_periods);
    register_test(&daycycle_tests, "Matching Period Overrides Description", test_room_description_uses_matching_period);
    register_test(&daycycle_tests, "Wraparound Period Handling", test_room_description_wraps_across_midnight);
    register_test(&daycycle_tests, "First Matching Period Takes Priority", test_room_description_prefers_first_match);
    register_test(&daycycle_tests, "Empty Periods Are Skipped", test_room_description_skips_empty_periods);
    register_test_group(&daycycle_tests);
}
