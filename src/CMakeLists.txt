#
# Mud98 Game Executable
#
cmake_minimum_required(VERSION 3.14)

# ============================================================================
# Dependencies
# ============================================================================
if (NOT MSVC)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

# Persistence layer options (must come before dependencies)
option(ENABLE_JSON_PERSISTENCE "Build JSON persistence support (requires Jansson)" ON)
option(ENABLE_ROM_OLC_PERSISTENCE "Build ROM-OLC persistence support (legacy text format)" ON)

# Conditionally fetch Jansson only if JSON persistence is enabled
if (ENABLE_JSON_PERSISTENCE)
    include(FetchContent)
    
    # Configure Jansson options before fetching
    set(JANSSON_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(JANSSON_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(JANSSON_WITHOUT_TESTS ON CACHE BOOL "" FORCE)
    
    FetchContent_Declare(
        jansson
        GIT_REPOSITORY https://github.com/akheron/jansson.git
        #GIT_TAG        v2.14.1  -- Put back when they fix CMake issues on Windows
        GIT_TAG        master
    )
    
    FetchContent_MakeAvailable(jansson)
    
    # Workaround: Jansson doesn't properly expose its include directories
    # Use BUILD_INTERFACE so it only applies during build, not install
    FetchContent_GetProperties(jansson)
    if(jansson_POPULATED)
        target_include_directories(jansson INTERFACE 
            $<BUILD_INTERFACE:${jansson_SOURCE_DIR}/src>
        )
    endif()
endif()

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL)

if (OPENSSL_FOUND)
    message("OpenSSL version ${OPENSSL_VERSION} found.")
    message("  OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
    message("  OPENSSL_INCLUDE_DIRS: ${OPENSSL_INCLUDE_DIRS}")
    message("  OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")
    message("  OPENSSL_SSL_LIBRARY: ${OPENSSL_SSL_LIBRARY}")
    message("  OPENSSL_CRYPTO_LIBRARY: ${OPENSSL_CRYPTO_LIBRARY}")
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
# Add persist subdirectory first so its object libraries are available
add_subdirectory(persist)

# Core game logic shared by main executable, tests, and benchmarks
add_library(Mud98Core STATIC "merc.h" "act_comm.h" "act_comm.c" "act_enter.h" 
    "act_enter.c" "act_info.h" "act_info.c" "act_move.h" "act_move.c" 
    "act_obj.h" "act_obj.c" "act_wiz.h" "act_wiz.c" "alias.h" "alias.c" 
    "array.h" "ban.h" "ban.c" "color.h" "color.c" "combat_ops.h" "combat_rom.c" 
    "comm.h" "comm.c" "config.h" 
    "config.c" "db.c" "digest.h" "digest.c" "effects.h" "effects.c" "fileutils.h" 
    "fileutils.c" "fight.h" "fight.c" "flags.c" "format.h" "format.c" "globals.c" "handler.h" 
    "handler.c" "healer.c" "lox.c" "interp.c" "lookup.c" "magic.c" "magic2.c" 
    "match.h" "match.c" "mem_watchpoint.h" "mem_watchpoint.c" 
    "mob_cmds.h" "mob_cmds.c" "mob_prog.h" 
    "mob_prog.c" "music.c" "note.h" "note.c" "pcg_basic.c" "recycle.c" "reload.h" 
    "reload.c" "rng.h" "rng.c" "save.h" 
    "save.c" "scan.c" "skills.h" "skills.c" "socket.h" "special.h" "special.c" 
    "spell_list.h" "stringbuffer.h" "stringbuffer.c" "tables.c" "tablesave.c" 
    "update.h" "update.c" "weather.h" "weather.c"

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
        $<$<BOOL:${OPENSSL_FOUND}>:${OPENSSL_INCLUDE_DIR}>
        $<$<BOOL:${ZLIB_FOUND}>:${ZLIB_INCLUDE_DIRS}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/persist
)

# Link dependencies
target_link_libraries(Mud98Core 
    PUBLIC
        # Persistence layer object libraries
        Mud98Persist
        $<$<BOOL:${ENABLE_ROM_OLC_PERSISTENCE}>:Mud98PersistRomOlc>
        $<$<BOOL:${ENABLE_JSON_PERSISTENCE}>:Mud98PersistJson>
        $<$<BOOL:${ENABLE_JSON_PERSISTENCE}>:jansson>
    PRIVATE
        $<$<BOOL:${OPENSSL_FOUND}>:$<IF:$<BOOL:${MSVC}>,OpenSSL::Crypto;OpenSSL::SSL;OpenSSL::applink,OpenSSL::SSL>>
        $<$<BOOL:${ZLIB_FOUND}>:ZLIB::ZLIB>
        $<$<NOT:$<BOOL:${MSVC}>>:Threads::Threads>
        $<$<NOT:$<BOOL:${MSVC}>>:m>
)

# Compiler-specific options
target_compile_options(Mud98Core PRIVATE
    # MSVC options
    $<$<C_COMPILER_ID:MSVC>:
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
        /wd4668  # Until MS fixes their own macros
        /wd4996  # Stupid deprecation warnings
        /wd4191  # Fix your SDK, Microsoft
    >
    # GCC/Clang options
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:
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
    $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:_GNU_SOURCE>
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
    "tests/mock.h" "tests/mock.c" "tests/mock_rng.c" "tests/mock_combat.c" 
    "tests/lox_tests.h" "tests/lox_tests.c" 
    "tests/lox_ext_tests.c" "tests/persist_tests.c" "tests/entity_tests.c" 
    "tests/container_tests.c" "tests/act_tests.c" "tests/act_comm_tests.c" 
    "tests/act_enter_tests.c" "tests/act_obj_tests.c" "tests/act_move_tests.c" 
    "tests/act_wiz_tests.c" "tests/act_wiz2_tests.c" "tests/act_wiz3_tests.c" 
    "tests/act_wiz4_tests.c" "tests/act_wiz5_tests.c" "tests/interp_tests.c" 
    "tests/fight_tests.c" "tests/damage_tests.c" "tests/tohit_tests.c" "tests/combat_state_tests.c" "tests/skills_tests.c" "tests/event_tests.c" 
    "tests/faction_tests.c" "tests/money_tests.c" "tests/buffer_tests.c" 
    "tests/stringbuffer_tests.c" "tests/mem_watchpoint_tests.c" 
    "tests/login_tests.c" "tests/player_persist_tests.c" "tests/quest_tests.c" 
    "tests/fmt_tests.c" "tests/theme_tests.c" "tests/util_tests.c"
    "tests/daycycle_tests.c" "tests/multihit_tests.c"
)

# Compiler-specific options
target_compile_options(Mud98Tests PRIVATE
    # MSVC options
    $<$<C_COMPILER_ID:MSVC>:
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
        /wd4668  # Until MS fixes their own macros
        /wd4996  # Stupid deprecation warnings
        /wd4191  # Fix your SDK, Microsoft
    >
    # GCC/Clang options
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:
        -Wall -Werror -Wextra -pedantic
        -Wno-unused-parameter -Wno-unknown-pragmas
    >
    # GCC-specific
    $<$<C_COMPILER_ID:GNU>:-std=gnu2x>
    # Clang-specific
    $<$<C_COMPILER_ID:Clang>:-std=c2x>
)

# Compile definitions
target_compile_definitions(Mud98Tests PRIVATE
    $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:_GNU_SOURCE>
    $<$<NOT:$<BOOL:${OPENSSL_FOUND}>>:NO_OPENSSL>
    $<$<AND:$<NOT:$<BOOL:${OPENSSL_FOUND}>>,$<BOOL:${CYGWIN}>>:NO_ENCRYPTION>
    $<$<NOT:$<BOOL:${ZLIB_FOUND}>>:NO_ZLIB>
)

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
        $<$<BOOL:${OPENSSL_FOUND}>:${OPENSSL_INCLUDE_DIR}>
        $<$<BOOL:${ZLIB_FOUND}>:${ZLIB_INCLUDE_DIRS}>
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
