#
# COM CMakeLists.txt
#
cmake_minimum_required (VERSION 3.10)

project(rom)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4 /WX /std:c17)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -std=gnu2x -D_GNU_SOURCE)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -std=c2x -D_GNU_SOURCE)
endif()

add_executable(rom act_comm.c act_enter.c act_info.c act_move.c act_obj.c 
    act_wiz.c alias.c ban.c comm.c const.c db.c db2.c effects.c fight.c flags.c
    handler.c healer.c interp.c lookup.c magic.c magic2.c music.c note.c 
    recycle.c save.c scan.c skills.c special.c tables.c update.c)

target_link_libraries(rom crypt)