include (CTest)
enable_testing()

add_executable(test_bra_fs test_bra_fs.cpp)
target_include_directories(test_bra_fs PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_bra_fs PRIVATE bra_fs)

add_test(NAME test_bra_fs.wildcards             COMMAND test_bra_fs test_bra_fs_wildcards)
add_test(NAME test_bra_fs.sfx_filename_adjust   COMMAND test_bra_fs test_bra_fs_sfx_filename_adjust)

#####################################################################################################

add_executable(test_bra test_bra.cpp)
add_dependencies(test_bra bra unbra bra_sfx)
add_test(NAME test_bra.no_output_file               COMMAND test_bra test_bra_no_output_file)
add_test(NAME test_bra.unbra                        COMMAND test_bra test_bra_unbra)
add_test(NAME test_bra.sfx_0                        COMMAND test_bra test_bra_sfx_0)
add_test(NAME test_bra.sfx_1                        COMMAND test_bra test_bra_sfx_1)
add_test(NAME test_bra.sfx_2                        COMMAND test_bra test_bra_sfx_2)
add_test(NAME test_bra.not_more_than_1_same_file    COMMAND test_bra test_bra_not_more_than_1_same_file)

# set_tests_properties(
#     test_bra_no_output_file
#     test_bra_unbra
#     test_bra_sfx_0
#     test_bra_sfx_1
#     test_bra_sfx_2
#     test_bra_not_more_than_1_same_file
#     PROPERTIES WORKING_DIRECTORY
#     ${CMAKE_CURRENT_BINARY_DIR}
# )

target_include_directories(test_bra PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_bra PRIVATE bra_fs)
foreach(tgt IN ITEMS bra unbra bra_sfx)
    add_custom_command(TARGET test_bra POST_BUILD 
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${tgt}>
            ${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()

foreach(fixture_i IN ITEMS test.txt)
            add_custom_command(TARGET test_bra POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${CMAKE_CURRENT_SOURCE_DIR}/${fixture_i}
                        ${CMAKE_CURRENT_BINARY_DIR}/${fixture_i}
            )
endforeach()
