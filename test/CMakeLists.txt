include (CTest)
enable_testing()

add_executable(test_bra test_bra.cpp)
target_include_directories(test_bra PRIVATE "../src")
add_test(NAME test_bra_unbra COMMAND test_bra test_bra_unbra)
add_test(NAME test_bra_sfx COMMAND test_bra test_bra_sfx)
add_custom_command(TARGET test_bra POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:bra>
        ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_command(TARGET test_bra POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:unbra>
        ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_command(TARGET test_bra POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:bra_sfx>
        ${CMAKE_CURRENT_BINARY_DIR}
)


set(FIXTURES test.txt)
foreach(fixture_i ${FIXTURES})
            add_custom_command(TARGET test_bra POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${CMAKE_CURRENT_SOURCE_DIR}/${fixture_i}
                        ${CMAKE_CURRENT_BINARY_DIR}/${fixture_i}
            )
endforeach()
