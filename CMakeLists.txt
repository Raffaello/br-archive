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
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif (MSVC)
    add_compile_options(/W4)
endif()

function(win32_wildcard target)
    if(NOT WIN32)
        return()
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # target_link_options(${target} PUBLIC "-print-file-name=CRT_glob.o")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        
    elseif (MSVC)
        target_link_options(${target} PUBLIC "setargv.obj") # wsetargv.obj
    endif()

endfunction()

configure_file(src/version.h.in src/version.h)

add_library(bra_log STATIC src/bra_log.c)

add_library(bra_wildcards STATIC src/bra_wildcards.cpp)
target_include_directories(bra_wildcards PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bra_wildcards PUBLIC bra_log)

add_library(bra_fs STATIC src/bra_fs.cpp)
target_link_libraries(bra_fs PUBLIC bra_log bra_wildcards)

add_library(bra_fs_c STATIC src/bra_fs_c.cpp)
target_link_libraries(bra_fs_c PRIVATE bra_fs)

add_library(lib_bra STATIC src/lib_bra.c)
target_include_directories(lib_bra
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src> # for configure_file (version.h)
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(lib_bra PRIVATE bra_log bra_fs_c)
set_target_properties(lib_bra PROPERTIES
    PREFIX ""
)

add_library(bra_prog STATIC src/BraProgram.cpp)
target_link_libraries(bra_prog PUBLIC lib_bra bra_fs)

add_executable(bra src/bra.cpp)
target_link_libraries(bra PRIVATE bra_prog)
win32_wildcard(bra)

add_executable(unbra src/unbra.cpp)
target_link_libraries(unbra PRIVATE bra_prog)
win32_wildcard(unbra)

add_executable(bra_sfx src/bra_sfx.cpp)
target_link_libraries(bra_sfx PRIVATE bra_prog)
win32_wildcard(bra_sfx)

set_target_properties(bra_sfx PROPERTIES
    OUTPUT_NAME "bra"
    SUFFIX ".sfx"
)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
