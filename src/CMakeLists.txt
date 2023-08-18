#
# Mud98 CMakeLists.txt
#
cmake_minimum_required (VERSION 3.10)

project(Mud98)

set(OPENSSL_USE_STATIC_LIBS true)
find_package(OpenSSL REQUIRED)

if (NOT MSVC)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

if (MSVC)
    add_compile_options(/W3 /std:c17)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D NOCRYPT)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -std=gnu2x -D_GNU_SOURCE)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -std=c2x -D_GNU_SOURCE)
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" 
    OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-unknown-pragmas)
endif()

add_executable(Mud98 merc.h act_comm.c act_enter.c act_info.c act_move.c 
    act_obj.c act_wiz.c alias.c ban.c benchmark.c bit.c cmdedit.c comm.h comm.c
    const.c db.c db2.c digest.h digest.c effects.c fight.c flags.c gedit.c
    handler.c healer.c  hedit.c  interp.c lookup.c magic.c magic2.c main.c  
    mem.c  mob_cmds.c mob_prog.c music.c note.c olc.c olc_act.c olc_save.c 
    pcg_basic.c pedit.c raedit.c  recycle.c save.c scan.c screen.c sedit.c  
    skedit.c skills.c special.c string_edit.c tables.c tablesave.c update.c 
    wizutil.v3.c
)

if (NOT MSVC)
    target_link_libraries(Mud98 Threads::Threads OpenSSL::SSL)
else()
    target_link_libraries(Mud98 OpenSSL::Crypto OpenSSL::SSL OpenSSL::applink)
endif()
