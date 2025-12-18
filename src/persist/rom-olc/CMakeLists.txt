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
)

target_include_directories(Mud98PersistRomOlc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/entities
        ${CMAKE_SOURCE_DIR}/src/data
        ${CMAKE_SOURCE_DIR}/src/lox
        $<$<BOOL:${OPENSSL_FOUND}>:${OPENSSL_INCLUDE_DIR}>
        $<$<BOOL:${ZLIB_FOUND}>:${ZLIB_INCLUDE_DIRS}>
)
