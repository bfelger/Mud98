#
# Mud98 CMakeLists.txt
#
cmake_minimum_required (VERSION 3.10)

set (CMAKE_C_STANDARD 23)

set (MSVC_C_STD "/std:c17")

project(Mud98)

set(OPENSSL_USE_STATIC_LIBS true)
find_package(OpenSSL REQUIRED)

if (NOT MSVC)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

if (MSVC)
    set(MSVC_DISABLED_WARNINGS
        # Nonstandard extension, function/data pointer conversion in expression
        "C4152"     # For openssl/applink.c.

         # Assignment within conditional expression
        "C4706"     # Microsoft can take my assigning conditionals out of my 
                    # cold, dead hands.
        
        # X bytes padding after data member 'Y'
        "C4820"     # As long as sizeof() reports the correct value (it does), 
                    # this is a non-issue.

        # No function prototype given: converting '()' to '(void)'"
        "C4255"     # C23 removes this requirement. Although MSVC is not 
                    # implementing C23, I would rather make MSVC conform to 
                    # GCC/Clang, and not the other way around.

        # Unreferenced formal parameter
        "C4100"     # Disabling this is the equivalent of GCC/Clang's  
                    # '-Wno-unused-parameter'.

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

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MSVC_C_STD} /Wall ${MSVC_DISABLED_WARNINGS_OPT}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -DNOCRYPT")
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -pedantic -Wno-unused-parameter -std=gnu2x -D_GNU_SOURCE")
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -pedantic -Wno-unused-parameter -std=c2x -D_GNU_SOURCE")
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" 
    OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
endif()

include_directories(${CMAKE_SOURCE_DIR})

add_executable(Mud98 merc.h act_comm.c act_enter.c act_info.c act_move.c 
    act_obj.c act_wiz.c alias.c ban.c benchmark.c bit.c cmdedit.c color.h 
    color.c comm.h comm.c const.c db.c db2.c digest.h digest.c effects.c fight.c
    flags.c gedit.c handler.c healer.c  hedit.c  interp.c lookup.c magic.c 
    magic2.c main.c mem.c  mob_cmds.c mob_prog.c music.c note.c olc.c olc_act.c 
    olc_save.c pcg_basic.c pedit.c raedit.c  recycle.c save.c scan.c screen.c 
    sedit.c skedit.c skills.c special.c string_edit.c tables.c tablesave.c 
    tests.h tests.c update.c wizutil.v3.c 
    
    entities/player_data.h entities/player_data.c
    entities/char_data.h entities/char_data.c
    entities/mob_prototype.h entities/mob_prototype.c
)

if (NOT MSVC)
    target_link_libraries(Mud98 Threads::Threads OpenSSL::SSL)
else()
    target_link_libraries(Mud98 OpenSSL::Crypto OpenSSL::SSL OpenSSL::applink)
endif()

message("Build summary:")
message("")
message("        Project name:     ${CMAKE_PROJECT_NAME}")
message("        Host type:        ${CMAKE_SYSTEM_NAME}")
message("        Compiler:         ${CMAKE_C_COMPILER}")
message("        Compiler ID:      ${CMAKE_C_COMPILER_ID}")
message("        CMAKE_VERSION:    ${CMAKE_VERSION}")
message("        CFLAGS:           ${CMAKE_C_FLAGS}")
message("        CFLAGS_DEBUG:     ${CMAKE_C_FLAGS_DEBUG}")
message("        CPPFLAGS:         ${CMAKE_CXX_FLAGS}")
message("        Build type:       ${CMAKE_BUILD_TYPE}")
message("")
message("        Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message("")
