#
# JSON Persistence Format
# Modern JSON-based format (requires Jansson library)
#

# Jansson is required for JSON persistence
if (NOT TARGET jansson)
    message(FATAL_ERROR "JSON persistence requires Jansson library. "
                        "Either enable it or disable ENABLE_JSON_PERSISTENCE.")
endif()

add_library(Mud98PersistJson OBJECT
    # Core JSON utilities
    persist_json.h
    persist_json.c
    
    # Area persistence
    ../area/json/area_persist_json.h
    ../area/json/area_persist_json.c
    
    # Class persistence
    ../class/json/class_persist_json.h
    ../class/json/class_persist_json.c
    
    # Race persistence
    ../race/json/race_persist_json.h
    ../race/json/race_persist_json.c
    
    # Theme persistence
    ../theme/json/theme_persist_json.h
    ../theme/json/theme_persist_json.c
    
    # Command persistence
    ../command/json/command_persist_json.h
    ../command/json/command_persist_json.c
    
    # Lox script persistence
    ../lox/json/lox_persist_json.h
    ../lox/json/lox_persist_json.c
    
    # Skill persistence
    ../skill/json/skill_persist_json.h
    ../skill/json/skill_persist_json.c
    
    # Social persistence
    ../social/json/social_persist_json.h
    ../social/json/social_persist_json.c
    
    # Tutorial persistence
    ../tutorial/json/tutorial_persist_json.h
    ../tutorial/json/tutorial_persist_json.c
    
    # Player persistence
    ../player/json/player_persist_json.c
)

target_include_directories(Mud98PersistJson
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

# Compiler-specific options
target_compile_options(Mud98PersistJson PRIVATE
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
target_compile_definitions(Mud98PersistJson PRIVATE
    $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:_GNU_SOURCE>
    $<$<NOT:$<BOOL:${OPENSSL_FOUND}>>:NO_OPENSSL>
    $<$<AND:$<NOT:$<BOOL:${OPENSSL_FOUND}>>,$<BOOL:${CYGWIN}>>:NO_ENCRYPTION>
    $<$<NOT:$<BOOL:${ZLIB_FOUND}>>:NO_ZLIB>
)

target_link_libraries(Mud98PersistJson PUBLIC jansson)
