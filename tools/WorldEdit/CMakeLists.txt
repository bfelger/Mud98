cmake_minimum_required(VERSION 3.14)

project(WorldEdit LANGUAGES NONE)

set(WORLDEDIT_ROOT "${CMAKE_CURRENT_LIST_DIR}")

find_program(NPM_EXECUTABLE npm)
find_program(CARGO_EXECUTABLE cargo)

if (NOT NPM_EXECUTABLE)
    set(WORLDEDIT_NPM_MESSAGE "npm not found. Install Node.js to use WorldEdit.")
    add_custom_target(worldedit_install
        COMMAND ${CMAKE_COMMAND} -E echo "${WORLDEDIT_NPM_MESSAGE}"
        WORKING_DIRECTORY "${WORLDEDIT_ROOT}"
    )
    add_custom_target(worldedit_dev
        COMMAND ${CMAKE_COMMAND} -E echo "${WORLDEDIT_NPM_MESSAGE}"
        WORKING_DIRECTORY "${WORLDEDIT_ROOT}"
    )
    add_custom_target(worldedit_build
        COMMAND ${CMAKE_COMMAND} -E echo "${WORLDEDIT_NPM_MESSAGE}"
        WORKING_DIRECTORY "${WORLDEDIT_ROOT}"
    )
else()
    add_custom_target(worldedit_install
        COMMAND ${NPM_EXECUTABLE} install
        WORKING_DIRECTORY "${WORLDEDIT_ROOT}"
    )
    if (CARGO_EXECUTABLE)
        add_custom_target(worldedit_dev
            COMMAND ${NPM_EXECUTABLE} run tauri dev
            WORKING_DIRECTORY "${WORLDEDIT_ROOT}"
        )
        add_custom_target(worldedit_build
            COMMAND ${NPM_EXECUTABLE} run tauri build
            WORKING_DIRECTORY "${WORLDEDIT_ROOT}"
        )
    else()
        set(WORLDEDIT_CARGO_MESSAGE "cargo not found. Install Rust to run WorldEdit.")
        add_custom_target(worldedit_dev
            COMMAND ${CMAKE_COMMAND} -E echo "${WORLDEDIT_CARGO_MESSAGE}"
            WORKING_DIRECTORY "${WORLDEDIT_ROOT}"
        )
        add_custom_target(worldedit_build
            COMMAND ${CMAKE_COMMAND} -E echo "${WORLDEDIT_CARGO_MESSAGE}"
            WORKING_DIRECTORY "${WORLDEDIT_ROOT}"
        )
    endif()
endif()
