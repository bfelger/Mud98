#
# Mud98 Persistence Layer
#

# ============================================================================
# Persistence Format Options
# ============================================================================
option(ENABLE_JSON_PERSISTENCE "Build JSON persistence support (requires Jansson)" ON)
option(ENABLE_ROM_OLC_PERSISTENCE "Build ROM-OLC persistence support (legacy text format)" ON)

# At least one format must be enabled
if (NOT ENABLE_JSON_PERSISTENCE AND NOT ENABLE_ROM_OLC_PERSISTENCE)
    message(FATAL_ERROR "At least one persistence format must be enabled. "
                        "Enable ENABLE_JSON_PERSISTENCE or ENABLE_ROM_OLC_PERSISTENCE.")
endif()

# ============================================================================
# Core Persistence Library (Format-agnostic)
# ============================================================================
add_library(Mud98Persist OBJECT
    # Core I/O abstractions
    persist_io.h
    persist_io_adapters.h
    persist_io_adapters.c
    persist_result.h
    
    # Domain-specific coordinators (dispatch to format implementations)
    area/area_persist.h
    area/area_persist.c
    race/race_persist.h
    race/race_persist.c
    class/class_persist.h
    class/class_persist.c
    theme/theme_persist.h
    theme/theme_persist.c
    command/command_persist.h
    command/command_persist.c
    lox/lox_persist.h
    lox/lox_persist.c
    skill/skill_persist.h
    skill/skill_persist.c
    social/social_persist.h
    social/social_persist.c
    tutorial/tutorial_persist.h
    tutorial/tutorial_persist.c
    player/player_persist.h
    player/player_persist.c
)

target_include_directories(Mud98Persist
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
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
target_compile_options(Mud98Persist PRIVATE
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
target_compile_definitions(Mud98Persist PRIVATE
    $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:_GNU_SOURCE>
    $<$<NOT:$<BOOL:${OPENSSL_FOUND}>>:NO_OPENSSL>
    $<$<AND:$<NOT:$<BOOL:${OPENSSL_FOUND}>>,$<BOOL:${CYGWIN}>>:NO_ENCRYPTION>
    $<$<NOT:$<BOOL:${ZLIB_FOUND}>>:NO_ZLIB>
)

# ============================================================================
# Format-Specific Implementations
# ============================================================================
if (ENABLE_ROM_OLC_PERSISTENCE)
    add_subdirectory(rom-olc)
    target_sources(Mud98Persist PRIVATE $<TARGET_OBJECTS:Mud98PersistRomOlc>)
    target_compile_definitions(Mud98Persist PUBLIC ENABLE_ROM_OLC_PERSISTENCE)
    message(STATUS "ROM-OLC persistence: ENABLED")
else()
    message(STATUS "ROM-OLC persistence: DISABLED")
endif()

if (ENABLE_JSON_PERSISTENCE)
    add_subdirectory(json)
    target_sources(Mud98Persist PRIVATE $<TARGET_OBJECTS:Mud98PersistJson>)
    target_compile_definitions(Mud98Persist PUBLIC ENABLE_JSON_PERSISTENCE)
    message(STATUS "JSON persistence: ENABLED")
else()
    message(STATUS "JSON persistence: DISABLED")
endif()
