set(JANSSON_SOURCES
    dtoa.c
    dump.c
    error.c
    hashtable.c
    hashtable_seed.c
    load.c
    memory.c
    pack_unpack.c
    strbuffer.c
    strconv.c
    utf.c
    value.c
    version.c
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/jansson_private_config.h
    ${CMAKE_CURRENT_BINARY_DIR}/jansson_config.h
    COPYONLY
)

add_library(jansson ${JANSSON_SOURCES})
target_compile_features(jansson PRIVATE c_std_99)

target_include_directories(jansson
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

if(WIN32 AND BUILD_SHARED_LIBS)
    target_sources(jansson PRIVATE jansson.def)
endif()

find_library(JANSSON_MATH_LIBRARY m)
if(JANSSON_MATH_LIBRARY)
    target_link_libraries(jansson PRIVATE ${JANSSON_MATH_LIBRARY})
endif()

set_target_properties(jansson PROPERTIES POSITION_INDEPENDENT_CODE ON)
