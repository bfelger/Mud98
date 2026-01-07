#
# Mud98 Base CMake Configuration
#

cmake_minimum_required(VERSION 3.14)

# Define the project at the root level
project(Mud98 VERSION 1.0 LANGUAGES C)

# Place built binaries under bin/<config> at the repo root so runtime files
# (e.g., mud98.cfg) are easy to find when launching from the root.
set(MUD98_RUNTIME_BASE "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MUD98_RUNTIME_BASE}")
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${cfg}" cfg_upper)
    set("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg_upper}" "${MUD98_RUNTIME_BASE}/${cfg}")
endforeach()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # Default to repo-local installs to avoid touching system prefixes.
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/bin" CACHE PATH "Install path prefix." FORCE)
endif()

# ============================================================================
# Testing & Benchmarking
# ============================================================================
enable_testing()

# Tests and benchmarks are built automatically as separate targets.
# Run with:
#   ctest                      # Run all tests via CTest
#   ./bin/<Config>/Mud98Tests  # Run tests directly
#   ./bin/<Config>/Mud98Benchmarks  # Run benchmarks directly

# ============================================================================
# Main Game
# ============================================================================
add_subdirectory(src)

# ============================================================================
# Helper Tools
# ============================================================================
# Add helper tool subdirectories here. Each tool can have completely
# different build settings, dependencies, and compiler flags from the main
# game. They are built as separate targets.
add_subdirectory(tools/mud98CombatSim)
