///////////////////////////////////////////////////////////////////////////////
// all_tests.c
///////////////////////////////////////////////////////////////////////////////

#include "tests.h"

#include "test_registry.h"

#include "lox_tests.h"

#include <lox/vm.h>
#include <lox/table.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

bool test_output_enabled = false;
bool test_act_output_enabled = false;
bool test_socket_output_enabled = false;

void run_all_tests();

void run_unit_tests()
{
    table_use_heap_allocator(&vm.strings);
    table_reserve(&vm.strings, 16384);

    register_lox_tests();
    register_lox_ext_tests();
    register_container_tests();
    register_entity_tests();
    register_act_tests();
    register_act_comm_tests();
    register_act_enter_tests();
    register_act_obj_tests();
    register_act_move_tests();
    register_act_wiz_tests();
    register_act_wiz2_tests();
    register_act_wiz3_tests();
    register_act_wiz4_tests();
    register_act_wiz5_tests();
    register_interp_tests();
    register_fight_tests();
    register_damage_tests();
    register_tohit_tests();
    register_combat_state_tests();
    register_skills_tests();
    register_fmt_tests();
    register_theme_tests();
    register_util_tests();
    register_event_tests();
    register_faction_tests();
    register_money_tests();
    register_buffer_tests();
    register_stringbuffer_tests();
    register_mem_watchpoint_tests();
    register_quest_tests();
    register_login_tests();
    register_persist_tests();
    register_player_persist_tests();
    register_daycycle_tests();
    register_multihit_tests();

    test_output_enabled = true;
    test_disassemble_on_error = true;

    run_all_tests();

    test_output_enabled = false;
    test_disassemble_on_error = false;
}
