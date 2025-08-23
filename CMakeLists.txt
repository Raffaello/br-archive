cmake_minimum_required(VERSION 3.27)

project(BRa VERSION 0.1.0 DESCRIPTION "BR-Archive")

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (CTest)
enable_testing()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX d)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(src/version.h.in src/version.h)

add_library(lib_bra STATIC src/lib_bra.c)
set_property(TARGET lib_bra PROPERTY COMPILE_WARNING_AS_ERROR ON)
target_include_directories(lib_bra 
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src> # for configure_file (version.h)
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(bra src/bra.cpp)
set_property(TARGET bra PROPERTY COMPILE_WARNING_AS_ERROR ON)
target_include_directories(bra PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src> # for configure_file (version.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
# target_sources(bra PRIVATE ...)
# target_compile_options(bra ...)
target_link_libraries(bra PRIVATE lib_bra)
