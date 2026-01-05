#
# ROM-OLC Persistence Format
# Legacy text-based format (.are, .olc files)
#

add_library(Mud98PersistRomOlc OBJECT
    # Area persistence
    ../area/rom-olc/area_persist_rom_olc.h
    ../area/rom-olc/area_persist_rom_olc.c
    
    # Class persistence
    ../class/rom-olc/class_persist_rom_olc.h
    ../class/rom-olc/class_persist_rom_olc.c
    
    # Race persistence
    ../race/rom-olc/race_persist_rom_olc.h
    ../race/rom-olc/race_persist_rom_olc.c
    
    # Theme persistence
    ../theme/rom-olc/theme_persist_rom_olc.h
    ../theme/rom-olc/theme_persist_rom_olc.c
    ../theme/rom-olc/theme_rom_olc_io.h
    ../theme/rom-olc/theme_rom_olc_io.c
    
    # Command persistence
    ../command/rom-olc/command_persist_rom_olc.h
    ../command/rom-olc/command_persist_rom_olc.c
    
    # Lox script persistence
    ../lox/rom-olc/lox_persist_rom_olc.h
    ../lox/rom-olc/lox_persist_rom_olc.c
    
    # Skill persistence
    ../skill/rom-olc/skill_persist_rom_olc.h
    ../skill/rom-olc/skill_persist_rom_olc.c
    
    # Social persistence
    ../social/rom-olc/social_persist_rom_olc.h
    ../social/rom-olc/social_persist_rom_olc.c
    
    # Tutorial persistence
    ../tutorial/rom-olc/tutorial_persist_rom_olc.h
    ../tutorial/rom-olc/tutorial_persist_rom_olc.c
    
    # Player persistence
    ../player/rom-olc/player_persist_rom_olc.c
    
    # Loot persistence
    ../loot/rom-olc/loot_persist_rom_olc.h
    ../loot/rom-olc/loot_persist_rom_olc.c
    
    # Recipe persistence
    ../recipe/rom-olc/recipe_persist_rom_olc.h
    ../recipe/rom-olc/recipe_persist_rom_olc.c
)

target_include_directories(Mud98PersistRomOlc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/entities
        ${CMAKE_SOURCE_DIR}/src/data
        ${CMAKE_SOURCE_DIR}/src/lox
        ${CMAKE_SOURCE_DIR}/src/craft
        $<$<BOOL:${OPENSSL_FOUND}>:${OPENSSL_INCLUDE_DIR}>
        $<$<BOOL:${ZLIB_FOUND}>:${ZLIB_INCLUDE_DIRS}>
)

# Compiler-specific options
target_compile_options(Mud98PersistRomOlc PRIVATE
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
target_compile_definitions(Mud98PersistRomOlc PRIVATE
    $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:_GNU_SOURCE>
    $<$<NOT:$<BOOL:${OPENSSL_FOUND}>>:NO_OPENSSL>
    $<$<AND:$<NOT:$<BOOL:${OPENSSL_FOUND}>>,$<BOOL:${CYGWIN}>>:NO_ENCRYPTION>
    $<$<NOT:$<BOOL:${ZLIB_FOUND}>>:NO_ZLIB>
)
