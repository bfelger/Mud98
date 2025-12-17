#
# Mud98 Game Executable
#

# ============================================================================
# Dependencies
# ============================================================================
if (NOT MSVC)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

add_subdirectory(jansson)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL)

if (OPENSSL_FOUND)
    message("OpenSSL version ${OPENSSL_VERSION} found at ${OPENSSL_INCLUDE_DIRS}.")
endif()

if (NOT OPENSSL_FOUND OR OPENSSL_VERSION VERSION_LESS "3.0.0")
    message(WARNING "OpenSSL 3.0 or higher not found. If you have it installed, set the OPENSSL_ROOT_DIR environment variable. "
                    "If you proceed without OpenSSL, you will not have TLS capability.")
    set(NO_OPENSSL TRUE)
endif()

set(ZLIB_USE_STATIC_LIBS TRUE)
find_package(ZLIB)
if (NOT ZLIB_FOUND)
    message(WARNING "ZLIB not found. If you have it installed, set the ZLIB_ROOT_DIR environment variable. "
                    "If you proceed without ZLIB, you will not be able to use MTH compression.")
endif()

# ============================================================================
# Mud98Core Library
# ============================================================================
# Core game logic shared by main executable, tests, and benchmarks
add_library(Mud98Core STATIC "merc.h" "act_comm.h" "act_comm.c" "act_enter.h" 
    "act_enter.c" "act_info.h" "act_info.c" "act_move.h" "act_move.c" 
    "act_obj.h" "act_obj.c" "act_wiz.h" "act_wiz.c" "alias.h" "alias.c" 
    "array.h" "ban.h" "ban.c" "color.h" "color.c" "comm.h" "comm.c" "config.h" 
    "config.c" "db.c" "digest.h" "digest.c" "effects.h" "effects.c" "fileutils.h" 
    "fileutils.c" "fight.h" "fight.c" "flags.c" "format.h" "format.c" "globals.c" "handler.h" 
    "handler.c" "healer.c" "lox.c" "interp.c" "lookup.c" "magic.c" "magic2.c" 
    "match.h" "match.c" "mem_watchpoint.h" "mem_watchpoint.c" 
    "mob_cmds.h" "mob_cmds.c" "mob_prog.h" 
    "mob_prog.c" "music.c" "note.h" "note.c" "pcg_basic.c" "recycle.c" "reload.h" 
    "reload.c" "save.h" 
    "save.c" "scan.c" "skills.h" "skills.c" "socket.h" "special.h" "special.c" 
    "spell_list.h" "stringbuffer.h" "stringbuffer.c" "tables.c" "tablesave.c" 
    "update.h" "update.c" "weather.h" "weather.c"

    "persist/persist_io.h" "persist/persist_io_adapters.h" 
    "persist/persist_io_adapters.c" "persist/persist_result.h" 
    "persist/area/area_persist.h" "persist/area/area_persist.c" 
    "persist/json/persist_json.h" "persist/json/persist_json.c"
    "persist/area/rom-olc/area_persist_rom_olc.h" 
    "persist/area/rom-olc/area_persist_rom_olc.c" "persist/area/json/area_persist_json.h" 
    "persist/area/json/area_persist_json.c" "persist/race/race_persist.h" 
    "persist/race/race_persist.c" "persist/class/class_persist.h" 
    "persist/class/class_persist.c" "persist/class/rom-olc/class_persist_rom_olc.h" 
    "persist/class/rom-olc/class_persist_rom_olc.c" 
    "persist/class/json/class_persist_json.h" "persist/class/json/class_persist_json.c" 
    "persist/race/rom-olc/race_persist_rom_olc.h" 
    "persist/race/rom-olc/race_persist_rom_olc.c" "persist/race/json/race_persist_json.h" 
    "persist/race/json/race_persist_json.c"
    "persist/theme/theme_persist.h" "persist/theme/theme_persist.c"
    "persist/theme/rom-olc/theme_persist_rom_olc.h" "persist/theme/rom-olc/theme_persist_rom_olc.c"
    "persist/theme/json/theme_persist_json.h" "persist/theme/json/theme_persist_json.c"
    "persist/theme/rom-olc/theme_rom_olc_io.h" "persist/theme/rom-olc/theme_rom_olc_io.c"
    "persist/command/command_persist.h" "persist/command/command_persist.c"
    "persist/command/rom-olc/command_persist_rom_olc.h" "persist/command/rom-olc/command_persist_rom_olc.c"
    "persist/command/json/command_persist_json.h" "persist/command/json/command_persist_json.c"
    "persist/lox/lox_persist.h" "persist/lox/lox_persist.c"
    "persist/lox/rom-olc/lox_persist_rom_olc.h" "persist/lox/rom-olc/lox_persist_rom_olc.c"
    "persist/lox/json/lox_persist_json.h" "persist/lox/json/lox_persist_json.c"
    "persist/skill/skill_persist.h" "persist/skill/skill_persist.c"
    "persist/skill/rom-olc/skill_persist_rom_olc.h" "persist/skill/rom-olc/skill_persist_rom_olc.c"
    "persist/skill/json/skill_persist_json.h" "persist/skill/json/skill_persist_json.c"
    "persist/social/social_persist.h" "persist/social/social_persist.c"
    "persist/social/rom-olc/social_persist_rom_olc.h" "persist/social/rom-olc/social_persist_rom_olc.c"
    "persist/social/json/social_persist_json.h" "persist/social/json/social_persist_json.c"
    "persist/tutorial/tutorial_persist.h" "persist/tutorial/tutorial_persist.c"
    "persist/tutorial/rom-olc/tutorial_persist_rom_olc.h" "persist/tutorial/rom-olc/tutorial_persist_rom_olc.c"
    "persist/tutorial/json/tutorial_persist_json.h" "persist/tutorial/json/tutorial_persist_json.c"
    "persist/player/player_persist.h" "persist/player/player_persist.c"
    "persist/player/rom-olc/player_persist_rom_olc.c"
    "persist/player/json/player_persist_json.c"

    "olc/aedit.c" "olc/bit.h" "olc/bit.c" "olc/cedit.c" "olc/cmdedit.c" 
    "olc/event_edit.h" "olc/event_edit.c" "olc/gedit.c" "olc/hedit.c" 
    "olc/medit.c" "olc/oedit.c" "olc/olc.h" "olc/olc.c" "olc/olc_act.c" 
    "olc/olc_save.c" "olc/pedit.c" "olc/period_edit.h" "olc/period_edit.c"
    "olc/qedit.c" "olc/raedit.c" "olc/redit.c" 
    "olc/screen.h" "olc/screen.c" "olc/sedit.c" "olc/skedit.c" "olc/tedit.c" 
    "olc/string_edit.h" "olc/string_edit.c"  "olc/lox_edit.h" "olc/lox_edit.c"
    "olc/scredit.c" "olc/theme_edit.c"

    "mth/mth.h" "mth/msdp.c" "mth/mth.c" "mth/telopt.c"
    
    "entities/affect.h" "entities/affect.c"
    "entities/area.h" "entities/area.c"
    "entities/descriptor.h" "entities/descriptor.c"
    "entities/entity.h" "entities/entity.c"
    "entities/event.h" "entities/event.c"
    "entities/extra_desc.h" "entities/extra_desc.c"
    "entities/faction.h" "entities/faction.c"
    "entities/help_data.h" "entities/help_data.c"
    "entities/mob_memory.h" "entities/mob_memory.c"
    "entities/mob_prototype.h" "entities/mob_prototype.c"
    "entities/mobile.h" "entities/mobile.c"
    "entities/obj_prototype.h" "entities/obj_prototype.c"
    "entities/object.h" "entities/object.c"
    "entities/player_data.h" "entities/player_data.c"
    "entities/reset.h" "entities/reset.c" 
    "entities/daycycle_period.h" "entities/daycycle_period.c"
    "entities/room.h" "entities/room.c"
    "entities/room_exit.h" "entities/room_exit.c"
    "entities/shop_data.h" "entities/shop_data.c"
    
    "data/class.h" "data/class.c"
    "data/damage.h" "data/damage.c"
    "data/direction.h" "data/direction.c"
    "data/events.h" "data/events.c"
    "data/item.h" "data/item.c"
    "data/mobile_data.h" "data/mobile_data.c"
    "data/player.h"
    "data/race.h" "data/race.c"
    "data/skill.h" "data/skill.c"
    "data/social.h" "data/social.c"
    "data/spell.h" "data/spell.c"
    "data/stats.h" "data/stats.c"
    "data/tutorial.h" "data/tutorial.c"
    "data/quest.h" "data/quest.c"

    "lox/lox.h" "lox/common.h"
    "lox/array.h" "lox/array.c"
    "lox/compiler.h" "lox/compiler.c"
    "lox/chunk.h" "lox/chunk.c"
    "lox/debug.h" "lox/debug.c"
    "lox/list.h" "lox/list.c"
    "lox/memory.h" "lox/memory.c"
    "lox/native.h" "lox/native.c"
    "lox/native_funcs.c" "lox/native_methods.c" "lox/native_cmds.c"
    "lox/object.h" "lox/object.c"
    "lox/ordered_table.h" "lox/ordered_table.c"
    "lox/scanner.h" "lox/scanner.c"
    "lox/table.h" "lox/table.c"
    "lox/value.h" "lox/value.c"
    "lox/vm.h" "lox/vm.c"
    "lox/function.h"
    "lox/segvec.h" "lox/segvec.c"
    "lox/enum.h" "lox/enum.c"

    "snippets/counter.c" "snippets/lore.c" "snippets/merc.dual.v1.c" 
    "snippets/wizutil.v3.c"
)

target_link_libraries(Mud98Core PRIVATE ${Mud98_LIBS})

# Configure library includes (PUBLIC so consumers get them)
target_include_directories(Mud98Core
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/entities
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_CURRENT_SOURCE_DIR}/lox
        ${CMAKE_CURRENT_SOURCE_DIR}/olc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/persist
)

# Link dependencies
target_link_libraries(Mud98Core 
    PUBLIC 
        jansson
    PRIVATE
        $<$<BOOL:${OPENSSL_FOUND}>:$<IF:$<BOOL:${MSVC}>,OpenSSL::Crypto;OpenSSL::SSL;OpenSSL::applink,OpenSSL::SSL>>
        $<$<BOOL:${ZLIB_FOUND}>:ZLIB::ZLIB>
        $<$<NOT:$<BOOL:${MSVC}>>:Threads::Threads>
        $<$<NOT:$<BOOL:${MSVC}>>:m>
)

# Compiler-specific options
target_compile_options(Mud98Core PRIVATE
    # MSVC options
    $<$<CXX_COMPILER_ID:MSVC>:
        /Wall /WX /std:c17
        /wd4061  # enumerator not explicitly handled
        /wd4068  # unknown pragma
        /wd4100  # unreferenced parameter
        /wd4152  # function/data pointer conversion
        /wd4255  # no function prototype
        /wd4706  # assignment in conditional
        /wd4710  # function not inlined
        /wd4820  # padding after data member
        /wd5045  # Spectre mitigation
    >
    # GCC/Clang options
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        -Wall -Werror -Wextra -pedantic
        -Wno-unused-parameter -Wno-unknown-pragmas
    >
    # GCC-specific
    $<$<C_COMPILER_ID:GNU>:-std=gnu2x>
    # Clang-specific
    $<$<C_COMPILER_ID:Clang>:-std=c2x>
)

# Compile definitions
target_compile_definitions(Mud98Core PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS NOCRYPT>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:_GNU_SOURCE>
    $<$<NOT:$<BOOL:${OPENSSL_FOUND}>>:NO_OPENSSL>
    $<$<AND:$<NOT:$<BOOL:${OPENSSL_FOUND}>>,$<BOOL:${CYGWIN}>>:NO_ENCRYPTION>
    $<$<NOT:$<BOOL:${ZLIB_FOUND}>>:NO_ZLIB>
)

# ============================================================================
# Mud98 Main Executable
# ============================================================================
add_executable(Mud98 "main.c" "test_stubs.c")
target_link_libraries(Mud98 PRIVATE Mud98Core)

# ============================================================================
# Mud98Tests Executable
# ============================================================================
add_executable(Mud98Tests
    "tests/test_main.c" "tests/tests.c" "tests/tests.h" "tests/all_tests.c" 
    "tests/test_registry.h" "tests/test_registry.c" 
    "tests/mock.h" "tests/mock.c" "tests/lox_tests.h" "tests/lox_tests.c" 
    "tests/lox_ext_tests.c" "tests/persist_tests.c" "tests/entity_tests.c" 
    "tests/container_tests.c" "tests/act_tests.c" "tests/act_comm_tests.c" 
    "tests/act_enter_tests.c" "tests/act_obj_tests.c" "tests/act_move_tests.c" 
    "tests/act_wiz_tests.c" "tests/act_wiz2_tests.c" "tests/act_wiz3_tests.c" 
    "tests/act_wiz4_tests.c" "tests/act_wiz5_tests.c" "tests/interp_tests.c" 
    "tests/fight_tests.c" "tests/skills_tests.c" "tests/event_tests.c" 
    "tests/faction_tests.c" "tests/money_tests.c" "tests/buffer_tests.c" 
    "tests/stringbuffer_tests.c" "tests/mem_watchpoint_tests.c" 
    "tests/login_tests.c" "tests/player_persist_tests.c" "tests/quest_tests.c" 
    "tests/fmt_tests.c" "tests/theme_tests.c" "tests/util_tests.c"
    "tests/daycycle_tests.c"
)
target_link_libraries(Mud98Tests PRIVATE Mud98Core)

# Register with CTest
add_test(NAME AllUnitTests COMMAND $<TARGET_FILE:Mud98Tests>)
set_tests_properties(AllUnitTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ============================================================================
# Mud98Benchmarks Executable
# ============================================================================
add_executable(Mud98Benchmarks
    "benchmarks/benchmark_main.c"
    "test_stubs.c"
    "benchmarks/benchmarks.h" "benchmarks/benchmarks.c" 
    "benchmarks/container_benchmarks.c" "benchmarks/format_benchmarks.c"
)
target_link_libraries(Mud98Benchmarks PRIVATE Mud98Core)

message("Config summary:")
message("")
message("        Project name:     ${CMAKE_PROJECT_NAME}")
message("        Host type:        ${CMAKE_SYSTEM_NAME}")
message("        Compiler:         ${CMAKE_C_COMPILER}")
message("        Compiler ID:      ${CMAKE_C_COMPILER_ID}")
message("        CMAKE_VERSION:    ${CMAKE_VERSION}")
message("        OpenSSL:          ${OPENSSL_FOUND}")
message("        ZLIB:             ${ZLIB_FOUND}")
message("        Build type:       ${CMAKE_BUILD_TYPE}")
message("")
message("        Install prefix:   ${CMAKE_INSTALL_PREFIX}")
message("")
