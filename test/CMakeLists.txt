include (CTest)
enable_testing()

add_executable(test_bra test_bra.cpp)
add_dependencies(test_bra bra unbra bra_sfx)
add_test(NAME test_bra_no_output_file               COMMAND test_bra test_bra_no_output_file)
add_test(NAME test_bra_unbra                        COMMAND test_bra test_bra_unbra)
add_test(NAME test_bra_sfx                          COMMAND test_bra test_bra_sfx)
add_test(NAME test_bra_not_more_than_1_same_file    COMMAND test_bra test_bra_not_more_than_1_same_file)

set_tests_properties(
    test_bra_unbra
    test_bra_sfx 
    PROPERTIES WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(test_bra PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

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
