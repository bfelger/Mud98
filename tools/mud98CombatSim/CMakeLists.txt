cmake_minimum_required(VERSION 3.14)

project(mud98CombatSim LANGUAGES C)

add_executable(mud98CombatSim
    src/main.c
    src/sim_config.c
    src/sim.c
    src/metrics.c
    ${CMAKE_SOURCE_DIR}/src/test_stubs.c
)

target_link_libraries(mud98CombatSim PRIVATE Mud98Core Mud98CompilerSettings)
