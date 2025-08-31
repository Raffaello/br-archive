cmake_minimum_required(VERSION 3.27)

project(BRa VERSION 0.3.0 DESCRIPTION "BR-Archive")

include (CTest)
enable_testing()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20) # C++23 not-supported in CI-Linux (Ubuntu 24) yet
set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
add_compile_options(-Wall -Wextra -Wpedantic)
endif()

configure_file(src/version.h.in src/version.h)

add_library(lib_bra STATIC src/lib_bra.c)
target_include_directories(lib_bra
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src> # for configure_file (version.h)
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_target_properties(lib_bra PROPERTIES
    PREFIX ""
)

add_library(bra_fs OBJECT src/bra_fs.cpp)


add_executable(bra src/bra.cpp)
target_link_libraries(bra PRIVATE lib_bra bra_fs)

add_executable(unbra src/unbra.cpp)
target_link_libraries(unbra PRIVATE lib_bra bra_fs)

add_executable(bra_sfx src/bra_sfx.cpp)
target_link_libraries(bra_sfx PRIVATE lib_bra)
set_target_properties(bra_sfx PROPERTIES
    OUTPUT_NAME "bra"
    SUFFIX ".sfx"
)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
