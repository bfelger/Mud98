////////////////////////////////////////////////////////////////////////////////
// daycycle_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <lox/array.h>
#include <lox/object.h>

#include <entities/descriptor.h>
#include <entities/room.h>

#include <db.h>
#include <handler.h>
#include <weather.h>

#include <string.h>

TestGroup daycycle_tests;

static void register_area_for_broadcast(AreaData* area)
{
    if (!area)
        return;
    write_value_array(&global_areas, OBJ_VAL(area));
}

static void unregister_area_for_broadcast(AreaData* area)
{
    if (!area)
        return;
    remove_array_value(&global_areas, OBJ_VAL(area));
}

static bool output_contains_text(const char* substring)
{
    if (substring == NULL || substring[0] == '\0')
        return false;
    if (!IS_STRING(test_output_buffer))
        return false;
    const char* buffer = AS_CSTRING(test_output_buffer);
    if (buffer == NULL)
        return false;
    return strstr(buffer, substring) != NULL;
}

static RoomData* setup_room(const char* desc)
{
    RoomData* room = mock_room_data(50000, NULL);
    free_string(room->description);
    room->description = str_dup(desc);
    return room;
}

static DayCyclePeriod* create_period(RoomData* room, const char* name, int start, int end, const char* desc, const char* enter_msg, const char* exit_msg)
{
    DayCyclePeriod* period = room_daycycle_period_add(room, name, start, end);
    if (desc != NULL) {
        free_string(period->description);
        period->description = str_dup(desc);
    }
    if (enter_msg != NULL) {
        free_string(period->enter_message);
        period->enter_message = str_dup(enter_msg);
    }
    if (exit_msg != NULL) {
        free_string(period->exit_message);
        period->exit_message = str_dup(exit_msg);
    }
    return period;
}

static void reset_weather_state(int hour)
{
    time_info.hour = hour;
    time_info.day = 0;
    time_info.month = 0;
    time_info.year = 0;
    weather_info.sky = SKY_CLOUDLESS;
    weather_info.sunlight = SUN_DARK;
    weather_info.mmhg = 1000;
    weather_info.change = 0;
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

    DayCyclePeriod* period = room_daycycle_period_add(room, "day", 6, 18);
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

    DayCyclePeriod* period = room_daycycle_period_add(room, "night", 20, 4);
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

    DayCyclePeriod* first = room_daycycle_period_add(room, "first", 0, 23);
    free_string(first->description);
    first->description = str_dup("First match.");

    DayCyclePeriod* second = room_daycycle_period_add(room, "second", 0, 23);
    free_string(second->description);
    second->description = str_dup("Second match.");

    ASSERT_STR_EQ("First match.", room_description_for_hour(room, 12));

    return 0;
}

static int test_room_description_skips_empty_periods()
{
    RoomData* room = setup_room("Default description.");

    room_daycycle_period_add(room, "blank", 0, 23);

    DayCyclePeriod* filled = room_daycycle_period_add(room, "filled", 0, 23);
    free_string(filled->description);
    filled->description = str_dup("Visible description.");

    ASSERT_STR_EQ("Visible description.", room_description_for_hour(room, 5));

    return 0;
}

static int test_period_transition_detection_handles_enter_and_exit()
{
    RoomData* room = setup_room("Default description.");

    create_period(room, "twilight", 6, 10, NULL, "Dawn arrives.\n\r", "Dusk falls.\n\r");

    ASSERT(room_has_period_message_transition(room, 5, 6));
    ASSERT(room_has_period_message_transition(room, 10, 11));
    ASSERT(!room_has_period_message_transition(room, 7, 8));

    return 0;
}

static int test_broadcast_period_messages_only_on_transitions()
{
    RoomData* room_data = mock_room_data(50010, NULL);
    create_period(room_data, "pulse", 6, 6, NULL, "The light brightens.\n\r", "The light fades.\n\r");

    Room* room = mock_room(50010, room_data, NULL);
    Mobile* player = mock_player("Watcher");
    transfer_mob(player, room);

    // Entering the window emits the enter message.
    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    broadcast_room_period_messages(5, 6);
    test_socket_output_enabled = false;
    ASSERT_OUTPUT_CONTAINS("brightens");
    test_output_buffer = NIL_VAL;

    // Hours outside the window stay silent.
    test_socket_output_enabled = true;
    broadcast_room_period_messages(7, 8);
    test_socket_output_enabled = false;
    ASSERT(!output_contains_text("The day has begun."));
    test_output_buffer = NIL_VAL;

    // Leaving the window emits the exit message.
    test_socket_output_enabled = true;
    broadcast_room_period_messages(6, 7);
    test_socket_output_enabled = false;
    ASSERT_OUTPUT_CONTAINS("fades");
    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_room_period_messages_without_description()
{
    RoomData* room_data = mock_room_data(50011, NULL);
    free_string(room_data->description);
    room_data->description = str_dup("Plain room.\n\r");
    Room* room = mock_room(50011, room_data, NULL);
    Mobile* player = mock_player("Timekeeper");
    transfer_mob(player, room);

    create_period(room_data, "chime", 6, 6, NULL, "The grandfather clock chimes six times.\n\r", NULL);

    ASSERT_STR_EQ("Plain room.\n\r", room_description_for_hour(room_data, 6));

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    broadcast_room_period_messages(5, 6);
    test_socket_output_enabled = false;
    ASSERT_OUTPUT_CONTAINS("grandfather clock chimes");

    return 0;
}

static int test_indoor_rooms_only_receive_period_messages()
{
    RoomData* room_data = mock_room_data(50020, NULL);
    room_data->room_flags |= ROOM_INDOORS;
    create_period(room_data, "dawn", 5, 5, NULL, "The hallway brightens softly.\n\r", NULL);

    Room* room = mock_room(50020, room_data, NULL);
    Mobile* player = mock_player("Listener");
    transfer_mob(player, room);
    ASSERT(player->desc != NULL);
    player->desc->connected = CON_PLAYING;
    mock_connect_player_descriptor(player);

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;
    reset_weather_state(4);

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    update_weather_info();
    test_socket_output_enabled = false;

    ASSERT_OUTPUT_EQ("The hallway brightens softly.\n\r");

    test_output_buffer = NIL_VAL;
    mock_disconnect_player_descriptor(player);
    time_info = saved_time;
    weather_info = saved_weather;

    return 0;
}

static int test_daycycle_message_respects_suppression_flag()
{
    RoomData* room_data = mock_room_data(50030, NULL);
    Room* room = mock_room(50030, room_data, NULL);
    Mobile* player = mock_player("Watcher");
    transfer_mob(player, room);
    ASSERT(player->desc != NULL);
    player->desc->connected = CON_PLAYING;
    mock_connect_player_descriptor(player);

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;

    // Baseline: receives the default weather message.
    room_data->suppress_daycycle_messages = false;
    reset_weather_state(4);
    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    update_weather_info();
    test_socket_output_enabled = false;
    ASSERT_OUTPUT_CONTAINS("The day has begun.");

    // Suppressed: no weather message delivered.
    room_data->suppress_daycycle_messages = true;
    reset_weather_state(4);
    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    update_weather_info();
    test_socket_output_enabled = false;
    ASSERT(!output_contains_text("The day has begun."));

    test_output_buffer = NIL_VAL;
    mock_disconnect_player_descriptor(player);
    time_info = saved_time;
    weather_info = saved_weather;

    return 0;
}

static int test_area_period_messages_reach_rooms()
{
    AreaData* area_data = mock_area_data();
    register_area_for_broadcast(area_data);
    RoomData* room_data = mock_room_data(50040, area_data);
    room_data->room_flags |= ROOM_INDOORS;
    Room* room = mock_room(50040, room_data, NULL);
    Mobile* player = mock_player("Observer");
    transfer_mob(player, room);
    ASSERT(player->desc != NULL);
    player->desc->connected = CON_PLAYING;
    mock_connect_player_descriptor(player);

    DayCyclePeriod* period = area_daycycle_period_add(area_data, "dawn", 5, 5);
    free_string(period->enter_message);
    period->enter_message = str_dup("A soft glow warms the halls.\n\r");

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;
    reset_weather_state(4);

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    update_weather_info();
    test_socket_output_enabled = false;

    ASSERT_OUTPUT_EQ("A soft glow warms the halls.\n\r");

    test_output_buffer = NIL_VAL;
    mock_disconnect_player_descriptor(player);
    time_info = saved_time;
    weather_info = saved_weather;
    unregister_area_for_broadcast(area_data);
    return 0;
}

static int test_room_suppression_blocks_area_periods()
{
    AreaData* area_data = mock_area_data();
    register_area_for_broadcast(area_data);
    RoomData* room_data = mock_room_data(50041, area_data);
    room_data->suppress_daycycle_messages = true;
    Room* room = mock_room(50041, room_data, NULL);
    Mobile* player = mock_player("Silent");
    transfer_mob(player, room);
    ASSERT(player->desc != NULL);
    player->desc->connected = CON_PLAYING;
    mock_connect_player_descriptor(player);

    DayCyclePeriod* period = area_daycycle_period_add(area_data, "dawn", 5, 5);
    free_string(period->enter_message);
    period->enter_message = str_dup("Sunrise colors streak the sky.\n\r");

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;
    reset_weather_state(4);

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    update_weather_info();
    test_socket_output_enabled = false;

    ASSERT(!output_contains_text("The day has begun."));

    test_output_buffer = NIL_VAL;
    mock_disconnect_player_descriptor(player);
    time_info = saved_time;
    weather_info = saved_weather;
    unregister_area_for_broadcast(area_data);
    return 0;
}

static int test_area_suppression_blocks_default_daycycle()
{
    AreaData* area_data = mock_area_data();
    register_area_for_broadcast(area_data);
    RoomData* room_data = mock_room_data(50042, area_data);
    Room* room = mock_room(50042, room_data, NULL);
    Mobile* player = mock_player("Watcher");
    transfer_mob(player, room);
    ASSERT(player->desc != NULL);
    player->desc->connected = CON_PLAYING;
    mock_connect_player_descriptor(player);

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;

    area_data->suppress_daycycle_messages = false;
    reset_weather_state(4);
    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    update_weather_info();
    test_socket_output_enabled = false;
    ASSERT_OUTPUT_CONTAINS("The day has begun.");

    area_data->suppress_daycycle_messages = true;
    reset_weather_state(4);
    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    update_weather_info();
    test_socket_output_enabled = false;
    ASSERT(!output_contains_text("The day has begun."));

    test_output_buffer = NIL_VAL;
    mock_disconnect_player_descriptor(player);
    time_info = saved_time;
    weather_info = saved_weather;
    unregister_area_for_broadcast(area_data);
    return 0;
}

void register_daycycle_tests()
{
#define REGISTER(n, f)  register_test(&daycycle_tests, (n), (f))

    init_test_group(&daycycle_tests, "DAYCYCLE TESTS");
    register_test_group(&daycycle_tests);

    REGISTER("Defaults Without Periods", test_room_description_defaults_without_periods);
    REGISTER("Matching Period Overrides Description", test_room_description_uses_matching_period);
    REGISTER("Wraparound Period Handling", test_room_description_wraps_across_midnight);
    REGISTER("First Matching Period Takes Priority", test_room_description_prefers_first_match);
    REGISTER("Empty Periods Are Skipped", test_room_description_skips_empty_periods);
    REGISTER("Period Transition Detection", test_period_transition_detection_handles_enter_and_exit);
    REGISTER("Room Period Transition Messages", test_broadcast_period_messages_only_on_transitions);
    REGISTER("Room Period Messages Without Description", test_room_period_messages_without_description);
    REGISTER("Indoor Rooms Only Get Period Messages", test_indoor_rooms_only_receive_period_messages);
    REGISTER("Daycycle Messages Suppression Room Flag", test_daycycle_message_respects_suppression_flag);
    REGISTER("Area Period Messages Reach Rooms", test_area_period_messages_reach_rooms);
    REGISTER("Room Suppression Blocks Area Periods", test_room_suppression_blocks_area_periods);
    REGISTER("Area Suppression Blocks Default Daycycle", test_area_suppression_blocks_default_daycycle);
    
#undef REGISTER
}
