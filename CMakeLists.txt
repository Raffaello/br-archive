cmake_minimum_required(VERSION 3.27)

project(BRa VERSION 0.3.0 DESCRIPTION "BR-Archive")

include (CTest)
enable_testing()

set(CMAKE_C_STANDARD 17)   # C23 not supported in VSCode
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
        #
    elseif (MSVC)
        target_link_options(${target} PUBLIC "setargv.obj") # wsetargv.obj
    endif()

endfunction()

###########################################################

configure_file(src/version.h.in src/version.h)

add_library(lib_bra STATIC src/lib_bra.c)
target_sources(lib_bra
    PRIVATE
        src/lib_bra_private.c
        src/lib_bra_crc32c.c

        src/log/bra_log.c

        src/bra_tree_dir.c

        src/fs/bra_fs.cpp
        src/fs/bra_wildcards.cpp
        src/fs/bra_fs_c.cpp

        src/io/lib_bra_io_file.c
        src/io/lib_bra_io_file_ctx.c

        src/encoders/bra_rle.c
)
target_include_directories(lib_bra
    ### TODO: select what is public (at the moment looks everything except lib_bra_private.h)
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src> # for configure_file (version.h)
)
set_target_properties(lib_bra PROPERTIES
    PREFIX ""
)

add_library(bra_prog STATIC src/prog/BraProgram.cpp)
target_include_directories(bra_prog PUBLIC src/prog)
target_link_libraries(bra_prog PUBLIC lib_bra)

add_executable(bra src/prog/bra.cpp)
target_link_libraries(bra PRIVATE bra_prog)
win32_wildcard(bra)

add_executable(unbra src/prog/unbra.cpp)
target_link_libraries(unbra PRIVATE bra_prog)
win32_wildcard(unbra)

add_executable(bra_sfx src/prog/bra_sfx.cpp)
target_link_libraries(bra_sfx PRIVATE bra_prog)
win32_wildcard(bra_sfx)
target_compile_options(bra_sfx PRIVATE
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:CXX,GNU>>:-Os -ffunction-sections -fdata-sections>

    # Clang/AppleClang: prefer -Oz for size
    # $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>>:-Oz>
    
    # GCC: use -Os for portability (Oz requires newer GCC)
    # $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:CXX,GNU>>:-Os>
    
    # ELF: enable section splitting to allow GC
    # $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>,$<NOT:$<PLATFORM_ID:Darwin>>>:-ffunction-sections -fdata-sections>
    # MSVC: /O1 (~Os) # MSVC: enable function/data COMDAT for better /OPT:REF/ICF
    # $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:CXX,MSVC>>:/O1>
    # $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:CXX,MSVC>>:/Gy>
    # $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:CXX,MSVC>>:/Gw>
)
target_link_options(bra_sfx PRIVATE
    $<$<AND:$<CONFIG:Release>,$<LINK_LANG_AND_ID:CXX,GNU,Clang>>:-s -Wl,--gc-sections>

    # AppleClang (Mach-O): dead strip and strip local symbols
    # $<$<AND:$<CONFIG:Release>,$<LINK_LANG_AND_ID:CXX,AppleClang>>:-Wl,-dead_strip>
    # $<$<AND:$<CONFIG:Release>,$<LINK_LANG_AND_ID:CXX,AppleClang>>:-Wl,-x>
    
    # GNU/Clang on non-Darwin: pass -s to linker
    # $<$<AND:$<CONFIG:Release>,$<LINK_LANG_AND_ID:CXX,GNU,Clang>,$<NOT:$<PLATFORM_ID:Darwin>>>:-s>
   
    # # ELF: garbage-collect unused sections
    # $<$<AND:$<CONFIG:Release>,$<LINK_LANG_AND_ID:CXX,GNU,Clang>,$<NOT:$<PLATFORM_ID:Darwin>>>:-Wl,--gc-sections>
    
    # MSVC: identical COMDAT folding and unreferenced elimination
    # $<$<AND:$<CONFIG:Release>,$<LINK_LANG_AND_ID:CXX,MSVC>>:/OPT:REF>
    # $<$<AND:$<CONFIG:Release>,$<LINK_LANG_AND_ID:CXX,MSVC>>:/OPT:ICF>
)
set_target_properties(bra_sfx PROPERTIES
    OUTPUT_NAME "bra"
    SUFFIX ".sfx"
)


if(BUILD_TESTING)
    add_subdirectory(test)
endif()
