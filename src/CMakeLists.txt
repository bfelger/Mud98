#
# COM CMakeLists.txt
#
cmake_minimum_required (VERSION 3.10)

project(rom)

if (MSVC)
    add_compile_options(/W3 /std:c17)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D NOCRYPT)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -std=gnu2x -D_GNU_SOURCE)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -std=c2x -D_GNU_SOURCE)
endif()

add_executable(rom act_comm.c act_enter.c act_info.c act_move.c act_obj.c 
    act_wiz.c alias.c ban.c comm.c const.c db.c db2.c effects.c fight.c flags.c
    handler.c healer.c interp.c lookup.c magic.c magic2.c music.c note.c 
    pcg_basic.c recycle.c save.c scan.c skills.c special.c tables.c 
    update.c)

if (NOT MSVC)
    target_link_libraries(rom crypt)
endif()