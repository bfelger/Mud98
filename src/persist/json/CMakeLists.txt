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
)

target_link_libraries(Mud98PersistJson PUBLIC jansson)
