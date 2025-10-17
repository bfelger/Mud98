#
# Mud98 CMakeLists.txt
#
cmake_minimum_required (VERSION 3.12)

# MSVC will ignore this
set(CMAKE_C_STANDARD 23)

# Most recent C standard MSVC will pretend to follow
set(MSVC_C_STD "/std:c17")

project(Mud98)

set(Mud98_LIBS "")

if (NOT MSVC)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    list(APPEND Mud98_LIBS Threads::Threads)
endif()

set (OPENSSL_USE_STATIC_LIBS true)
find_package(OpenSSL)

if (OPENSSL_FOUND)
    message("OpenSSL version ${OPENSSL_VERSION} found at ${OPENSSL_INLCUDE_DIRS}.")
endif()

if (NOT OPENSSL_FOUND OR OPENSSL_VERSION VERSION_LESS "3.0.0")
    message(WARNING "OpenSSL 3.0 or higher not found. If you have it installed, set the OPENSSL_ROOT_DIR environment variable. "
                    "If you proceed without OpenSSL, you will not have TLS capability.")
    set(NO_OPENSSL true)
else()
    if (MSVC)
        list(APPEND Mud98_LIBS OpenSSL::Crypto OpenSSL::SSL OpenSSL::applink)
    else()
        list(APPEND Mud98_LIBS OpenSSL::SSL)
    endif()
endif()

set (ZLIB_USE_STATIC_LIBS true)
find_package(ZLIB)
if (ZLIB_FOUND)
    list(APPEND Mud98_LIBS ZLIB::ZLIB)
else()
    message(WARNING "ZLIB not found. If you have it installed, set the ZLIB_ROOT_DIR environment variable. "
                    "If you proceed without ZLIB, you will not be able to use MTH compression.")
    set(NO_ZLIB true)
endif()

if (MSVC)
    set(MSVC_DISABLED_WARNINGS

        # enumerator 'X' in switch of enum 'Y' is not explicitly handled by a 
        # case label
        "C4061"     # If no default is specified, we get C4062 intead. That is
                    # the only warning I care about.

        # unknown pragma 'GCC'
        "C4068"     # Equivalent to -Wno-unknown-pragmas

        # Unreferenced formal parameter
        "C4100"     # Disabling this is the equivalent of GCC/Clang's  
                    # '-Wno-unused-parameter'.

        # Nonstandard extension, function/data pointer conversion in expression
        "C4152"     # For openssl/applink.c.

        # No function prototype given: converting '()' to '(void)'"
        "C4255"     # C23 removes this requirement. Although MSVC is not 
                    # implementing C23, I would rather make MSVC conform to 
                    # GCC/Clang, and not the other way around.

         # Assignment within conditional expression
        "C4706"     # Microsoft can take my assigning conditionals out of my 
                    # cold, dead hands.

        # Function not inlined
        "C4710"     # Many stdlib calls are marked inline, but cannot be inlined
                    # at the call-site. This is noise.
        
        # X bytes padding after data member 'Y'
        "C4820"     # As long as sizeof() reports the correct value (it does), 
                    # this is a non-issue.

        # Compiler will insert Spectre mitigation for memory load if /Qspectre
        # switch specified
        "C5045"     # Spectre is serious business, but mitigations are costly.
                    # In the end, the choice to add mitigation is yours. Add
                    # /Qspectre to CFLAGS if you want to enable it. This WILL
                    # impact performance, as ROM does quite a bit of the things
                    # that need to be guarded. You can also mitigate by running
                    # as a user account with no privileges (which you should be
                    # doing, anyway). With or without Spectre, ASSUME HACKERS
                    # WILL BREACH. Prepare accordingly.
    )

    string(REPLACE "C" " -wd" MSVC_DISABLED_WARNINGS_OPT ${MSVC_DISABLED_WARNINGS})
    string(REGEX REPLACE "[/-]W[1234][ ]?" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MSVC_C_STD} /Wall /WX ${MSVC_DISABLED_WARNINGS_OPT}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -DNOCRYPT")
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu2x")
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c2x")
endif()

if (NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -pedantic -Wno-unused-parameter -Wno-unknown-pragmas -D_GNU_SOURCE")
endif()

if (NO_OPENSSL)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_OPENSSL")
    if (CYGWIN)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_ENCRYPTION")
    endif()
endif()

if (NOT ZLIB_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_ZLIB")
endif()

include_directories(${CMAKE_SOURCE_DIR})

add_executable(Mud98 merc.h act_comm.h act_comm.c act_enter.h act_enter.c
    act_info.h act_info.c act_move.h act_move.c act_obj.h act_obj.c act_wiz.h
    act_wiz.c alias.h alias.c array.h ban.h ban.c benchmark.h benchmark.c 
    color.h color.c comm.h comm.c config.h config.c db.c digest.h digest.c
    effects.h effects.c file.h file.c fight.h fight.c flags.c handler.h 
    handler.c healer.c interp.c lookup.c magic.c magic2.c main.c mob_cmds.h 
    mob_cmds.c mob_prog.h mob_prog.c music.c note.h note.c pcg_basic.c recycle.c
    save.h save.c scan.c skills.h skills.c socket.h special.h special.c 
    spell_list.h tables.c tablesave.c tests.h tests.c update.h update.c 
    weather.h weather.c wizutil.v3.c

    lox.c

    olc/aedit.c olc/bit.h olc/bit.c olc/cedit.c olc/cmdedit.c olc/gedit.c 
    olc/hedit.c olc/medit.c olc/oedit.c olc/olc.h olc/olc.c olc/olc_act.c 
    olc/olc_save.c olc/pedit.c olc/qedit.c olc/raedit.c olc/redit.c olc/screen.h 
    olc/screen.c olc/sedit.c olc/skedit.c olc/string_edit.h olc/string_edit.c

    mth/mth.h mth/msdp.c mth/mth.c mth/telopt.c
    
    entities/affect.h entities/affect.c
    entities/area.h entities/area.c
    entities/descriptor.h entities/descriptor.c
    entities/entity.h entities/entity.c
    entities/extra_desc.h entities/extra_desc.c
    entities/help_data.h entities/help_data.c
    entities/mob_memory.h entities/mob_memory.c
    entities/mob_prototype.h entities/mob_prototype.c
    entities/mobile.h entities/mobile.c
    entities/obj_prototype.h entities/obj_prototype.c
    entities/object.h entities/object.c
    entities/player_data.h entities/player_data.c
    entities/reset.h entities/reset.c 
    entities/room.h entities/room.c
    entities/room_exit.h entities/room_exit.c
    entities/shop_data.h entities/shop_data.c
    
    data/class.h data/class.c
    data/damage.h data/damage.c
    data/direction.h data/direction.c
    data/events.h data/events.c
    data/item.h data/item.c
    data/mobile_data.h data/mobile_data.c
    data/player.h
    data/race.h data/race.c
    data/skill.h data/skill.c
    data/social.h data/social.c
    data/spell.h data/spell.c
    data/stats.h data/stats.c
    data/quest.h data/quest.c

    lox/lox.h lox/common.h
    lox/array.h lox/array.c
    lox/compiler.h lox/compiler.c
    lox/chunk.h lox/chunk.c
    lox/debug.h lox/debug.c
    lox/list.h lox/list.c
    lox/memory.h lox/memory.c
    lox/native.h lox/native.c
    lox/native_funcs.c lox/native_methods.c lox/native_cmds.c
    lox/object.h lox/object.c
    lox/scanner.h lox/scanner.c
    lox/table.h lox/table.c
    lox/value.h lox/value.c
    lox/vm.h lox/vm.c
    lox/function.h
 
    tests/tests.c
    tests/test_registry.h tests/test_registry.c 
    tests/all_tests.c tests/tests.h
    tests/lox_tests.h tests/lox_tests.c tests/lox_ext_tests.c
    tests/entity_tests.c
    tests/mock.h tests/mock.c
)

target_link_libraries(Mud98 PRIVATE ${Mud98_LIBS})

message("Config summary:")
message("")
message("        Project name:     ${CMAKE_PROJECT_NAME}")
message("        Host type:        ${CMAKE_SYSTEM_NAME}")
message("        Compiler:         ${CMAKE_C_COMPILER}")
message("        Compiler ID:      ${CMAKE_C_COMPILER_ID}")
message("        CMAKE_VERSION:    ${CMAKE_VERSION}")
message("        CFLAGS:           ${CMAKE_C_FLAGS}")
message("        CFLAGS_DEBUG:     ${CMAKE_C_FLAGS_DEBUG}")
message("        CPPFLAGS:         ${CMAKE_CXX_FLAGS}")
message("        Linked Libs:      ${Mud98_LIBS}")
message("        Include dirs:     ${CMAKE_INCLUDE_PATH}")
message("        Build type:       ${CMAKE_BUILD_TYPE}")
message("")
message("        Install prefix:   ${CMAKE_INSTALL_PREFIX}")
message("")
