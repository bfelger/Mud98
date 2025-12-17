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
