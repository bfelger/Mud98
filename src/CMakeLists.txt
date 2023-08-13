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
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D NOCRYPT) # -DFIRST_BOOT)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -std=gnu2x -D_GNU_SOURCE) # -DFIRST_BOOT)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -std=c2x -D_GNU_SOURCE) # -DFIRST_BOOT)
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" 
    OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-unknown-pragmas)
endif()

add_executable(Mud98 act_comm.c act_enter.c act_info.c act_move.c act_obj.c 
    act_wiz.c alias.c ban.c comm.c const.c db.c db2.c effects.c fight.c flags.c
    handler.c healer.c interp.c lookup.c magic.c magic2.c main.c music.c note.c 
    pcg_basic.c recycle.c save.c scan.c skills.c special.c tables.c 
    update.c benchmark.c digest.c merc.h comm.h digest.h
    mob_cmds.c mob_prog.c olc.c olc_act.c olc_save.c bit.c
    mem.c string_edit.c pedit.c hedit.c cmdedit.c raedit.c skedit.c
    sedit.c gedit.c tablesave.c screen.c
)

if (NOT MSVC)
    target_link_libraries(Mud98 Threads::Threads OpenSSL::SSL)
else()
    target_link_libraries(Mud98 OpenSSL::Crypto OpenSSL::SSL OpenSSL::applink)
endif()
