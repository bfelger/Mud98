////////////////////////////////////////////////////////////////////////////////
// daycycle_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <lox/array.h>
#include <lox/object.h>

#include <entities/area.h>
#include <entities/descriptor.h>
#include <entities/event.h>
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

static int test_prdstart_event_on_room()
{
    AreaData* area_data = mock_area_data();
    register_area_for_broadcast(area_data);
    RoomData* room_data = mock_room_data(50100, area_data);
    Room* room = mock_room(50100, room_data, NULL);

    // Create a period for morning (hour 6-12)
    DayCyclePeriod* period = room_daycycle_period_add(room_data, "morning", 6, 12);
    free_string(period->enter_message);
    period->enter_message = str_dup("Morning light fills the area.\n\r");

    // Attach a TRIG_PRDSTART event to the room
    const char* event_src =
        "on_prdstart() {"
        "   print \"Morning period started!\";"
        "}";
    ObjClass* room_class = create_entity_class((Entity*)room,
        "room_50100", event_src);
    room->header.klass = room_class;
    init_entity_class((Entity*)room);

    Event* prdstart_event = new_event();
    prdstart_event->trigger = TRIG_PRDSTART;
    prdstart_event->method_name = lox_string("on_prdstart");
    prdstart_event->criteria = OBJ_VAL(lox_string("morning"));
    add_event((Entity*)room, prdstart_event);

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;
    reset_weather_state(5);  // Hour before morning starts

    test_output_buffer = NIL_VAL;
    update_weather_info();  // Advance to hour 6

    char* expected = "Morning period started!\n";
    ASSERT_OUTPUT_EQ(expected);

    test_output_buffer = NIL_VAL;
    time_info = saved_time;
    weather_info = saved_weather;
    unregister_area_for_broadcast(area_data);
    return 0;
}

static int test_prdstop_event_on_room()
{
    AreaData* area_data = mock_area_data();
    register_area_for_broadcast(area_data);
    RoomData* room_data = mock_room_data(50101, area_data);
    Room* room = mock_room(50101, room_data, NULL);

    // Create a period for morning (hour 6-12)
    DayCyclePeriod* period = room_daycycle_period_add(room_data, "morning", 6, 12);
    free_string(period->exit_message);
    period->exit_message = str_dup("Morning fades into afternoon.\n\r");

    // Attach a TRIG_PRDSTOP event to the room
    const char* event_src =
        "on_prdstop() {"
        "   print \"Morning period ended!\";"
        "}";
    ObjClass* room_class = create_entity_class((Entity*)room,
        "room_50101", event_src);
    room->header.klass = room_class;
    init_entity_class((Entity*)room);

    Event* prdstop_event = new_event();
    prdstop_event->trigger = TRIG_PRDSTOP;
    prdstop_event->method_name = lox_string("on_prdstop");
    prdstop_event->criteria = OBJ_VAL(lox_string("morning"));
    add_event((Entity*)room, prdstop_event);

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;
    reset_weather_state(12);  // Last hour of morning

    test_output_buffer = NIL_VAL;
    update_weather_info();  // Advance to hour 13

    char* expected = "Morning period ended!\n";
    ASSERT_OUTPUT_EQ(expected);

    test_output_buffer = NIL_VAL;
    time_info = saved_time;
    weather_info = saved_weather;
    unregister_area_for_broadcast(area_data);
    return 0;
}

static int test_prdstart_event_on_area()
{
    AreaData* area_data = mock_area_data();
    register_area_for_broadcast(area_data);
    
    // Create a single Area instance for the AreaData
    Area* area_inst = create_area_instance(area_data, false);

    // Create a period for evening (hour 18-22)
    DayCyclePeriod* period = area_daycycle_period_add(area_data, "evening", 18, 22);
    free_string(period->enter_message);
    period->enter_message = str_dup("Evening descends upon the land.\n\r");

    // Attach a TRIG_PRDSTART event to the area instance
    const char* event_src =
        "on_prdstart() {"
        "   print \"Evening period started in area!\";"
        "}";
    ObjClass* area_class = create_entity_class((Entity*)area_inst,
        "area_instance", event_src);
    area_inst->header.klass = area_class;
    init_entity_class((Entity*)area_inst);

    Event* prdstart_event = new_event();
    prdstart_event->trigger = TRIG_PRDSTART;
    prdstart_event->method_name = lox_string("on_prdstart");
    prdstart_event->criteria = OBJ_VAL(lox_string("evening"));
    add_event((Entity*)area_inst, prdstart_event);

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;
    reset_weather_state(17);  // Hour before evening starts

    test_output_buffer = NIL_VAL;
    update_weather_info();  // Advance to hour 18

    char* expected = "Evening period started in area!\n";
    ASSERT_OUTPUT_EQ(expected);

    test_output_buffer = NIL_VAL;
    time_info = saved_time;
    weather_info = saved_weather;
    unregister_area_for_broadcast(area_data);
    return 0;
}

static int test_prdstop_event_on_area()
{
    AreaData* area_data = mock_area_data();
    register_area_for_broadcast(area_data);
    
    // Create a single Area instance for the AreaData
    Area* area_inst = create_area_instance(area_data, false);

    // Create a period for evening (hour 18-22)
    DayCyclePeriod* period = area_daycycle_period_add(area_data, "evening", 18, 22);
    free_string(period->exit_message);
    period->exit_message = str_dup("Evening gives way to night.\n\r");

    // Attach a TRIG_PRDSTOP event to the area instance
    const char* event_src =
        "on_prdstop() {"
        "   print \"Evening period ended in area!\";"
        "}";
    ObjClass* area_class = create_entity_class((Entity*)area_inst,
        "area_instance", event_src);
    area_inst->header.klass = area_class;
    init_entity_class((Entity*)area_inst);

    Event* prdstop_event = new_event();
    prdstop_event->trigger = TRIG_PRDSTOP;
    prdstop_event->method_name = lox_string("on_prdstop");
    prdstop_event->criteria = OBJ_VAL(lox_string("evening"));
    add_event((Entity*)area_inst, prdstop_event);

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;
    reset_weather_state(22);  // Last hour of evening

    test_output_buffer = NIL_VAL;
    update_weather_info();  // Advance to hour 23

    char* expected = "Evening period ended in area!\n";
    ASSERT_OUTPUT_EQ(expected);

    test_output_buffer = NIL_VAL;
    time_info = saved_time;
    weather_info = saved_weather;
    unregister_area_for_broadcast(area_data);
    return 0;
}

static int test_prdstart_fires_after_message()
{
    AreaData* area_data = mock_area_data();
    register_area_for_broadcast(area_data);
    RoomData* room_data = mock_room_data(50102, area_data);
    Room* room = mock_room(50102, room_data, NULL);

    Mobile* player = mock_player("Observer");
    transfer_mob(player, room);
    ASSERT(player->desc != NULL);
    player->desc->connected = CON_PLAYING;
    mock_connect_player_descriptor(player);

    // Create a period with an enter message
    DayCyclePeriod* period = room_daycycle_period_add(room_data, "dawn", 5, 5);
    free_string(period->enter_message);
    period->enter_message = str_dup("Dawn breaks.\n\r");

    // Attach a TRIG_PRDSTART event that outputs text
    const char* event_src =
        "on_prdstart() {"
        "   print \"Event fired after message.\";"
        "}";
    ObjClass* room_class = create_entity_class((Entity*)room,
        "room_50102", event_src);
    room->header.klass = room_class;
    init_entity_class((Entity*)room);

    Event* prdstart_event = new_event();
    prdstart_event->trigger = TRIG_PRDSTART;
    prdstart_event->method_name = lox_string("on_prdstart");
    prdstart_event->criteria = OBJ_VAL(lox_string("dawn"));
    add_event((Entity*)room, prdstart_event);

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;
    reset_weather_state(4);

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    update_weather_info();
    test_socket_output_enabled = false;

    // Verify order: message should appear before event output
    char* output = AS_STRING(test_output_buffer)->chars;
    char* dawn_pos = strstr(output, "Dawn breaks.");
    char* event_pos = strstr(output, "Event fired after message.");
    
    ASSERT(dawn_pos != NULL);
    ASSERT(event_pos != NULL);
    ASSERT(dawn_pos < event_pos);  // Message should come before event

    test_output_buffer = NIL_VAL;
    mock_disconnect_player_descriptor(player);
    time_info = saved_time;
    weather_info = saved_weather;
    unregister_area_for_broadcast(area_data);
    return 0;
}

static int test_prdstop_fires_before_message()
{
    AreaData* area_data = mock_area_data();
    register_area_for_broadcast(area_data);
    RoomData* room_data = mock_room_data(50103, area_data);
    Room* room = mock_room(50103, room_data, NULL);

    Mobile* player = mock_player("Observer");
    transfer_mob(player, room);
    ASSERT(player->desc != NULL);
    player->desc->connected = CON_PLAYING;
    mock_connect_player_descriptor(player);

    // Create a period with an exit message
    DayCyclePeriod* period = room_daycycle_period_add(room_data, "dawn", 5, 5);
    free_string(period->exit_message);
    period->exit_message = str_dup("Dawn ends.\n\r");

    // Attach a TRIG_PRDSTOP event that outputs text
    const char* event_src =
        "on_prdstop() {"
        "   print \"Event fired before message.\";"
        "}";
    ObjClass* room_class = create_entity_class((Entity*)room,
        "room_50103", event_src);
    room->header.klass = room_class;
    init_entity_class((Entity*)room);

    Event* prdstop_event = new_event();
    prdstop_event->trigger = TRIG_PRDSTOP;
    prdstop_event->method_name = lox_string("on_prdstop");
    prdstop_event->criteria = OBJ_VAL(lox_string("dawn"));
    add_event((Entity*)room, prdstop_event);

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;
    reset_weather_state(5);  // At dawn

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    update_weather_info();  // Exit dawn
    test_socket_output_enabled = false;

    // Verify order: event output should appear before message
    char* output = AS_STRING(test_output_buffer)->chars;
    char* event_pos = strstr(output, "Event fired before message.");
    char* dawn_pos = strstr(output, "Dawn ends.");
    
    ASSERT(event_pos != NULL);
    ASSERT(dawn_pos != NULL);
    ASSERT(event_pos < dawn_pos);  // Event should come before message

    test_output_buffer = NIL_VAL;
    mock_disconnect_player_descriptor(player);
    time_info = saved_time;
    weather_info = saved_weather;
    unregister_area_for_broadcast(area_data);
    return 0;
}

static int test_prdstart_only_fires_for_matching_period()
{
    AreaData* area_data = mock_area_data();
    register_area_for_broadcast(area_data);
    RoomData* room_data = mock_room_data(50104, area_data);
    Room* room = mock_room(50104, room_data, NULL);

    // Create two periods
    room_daycycle_period_add(room_data, "morning", 6, 12);
    room_daycycle_period_add(room_data, "afternoon", 13, 17);

    // Attach event that only triggers for "afternoon"
    const char* event_src =
        "on_prdstart() {"
        "   print \"Afternoon event triggered!\";"
        "}";
    ObjClass* room_class = create_entity_class((Entity*)room,
        "room_50104", event_src);
    room->header.klass = room_class;
    init_entity_class((Entity*)room);

    Event* prdstart_event = new_event();
    prdstart_event->trigger = TRIG_PRDSTART;
    prdstart_event->method_name = lox_string("on_prdstart");
    prdstart_event->criteria = OBJ_VAL(lox_string("afternoon"));
    add_event((Entity*)room, prdstart_event);

    TimeInfo saved_time = time_info;
    WeatherInfo saved_weather = weather_info;
    
    // Enter morning - should NOT trigger
    reset_weather_state(5);
    test_output_buffer = NIL_VAL;
    update_weather_info();
    ASSERT(!output_contains_text("Afternoon event triggered!"));

    // Enter afternoon - SHOULD trigger
    reset_weather_state(12);
    test_output_buffer = NIL_VAL;
    update_weather_info();
    ASSERT(output_contains_text("Afternoon event triggered!"));

    test_output_buffer = NIL_VAL;
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
    REGISTER("PRDSTART Event On Room", test_prdstart_event_on_room);
    REGISTER("PRDSTOP Event On Room", test_prdstop_event_on_room);
    REGISTER("PRDSTART Event On Area", test_prdstart_event_on_area);
    REGISTER("PRDSTOP Event On Area", test_prdstop_event_on_area);
    REGISTER("PRDSTART Fires After Message", test_prdstart_fires_after_message);
    REGISTER("PRDSTOP Fires Before Message", test_prdstop_fires_before_message);
    REGISTER("PRDSTART Only Fires For Matching Period", test_prdstart_only_fires_for_matching_period);
    
#undef REGISTER
}
