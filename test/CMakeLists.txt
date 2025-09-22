include (CTest)
enable_testing()

add_executable(test_lib_bra test_lib_bra.cpp)
target_link_libraries(test_lib_bra PRIVATE lib_bra)

add_test(NAME test_lib_bra.test_lib_bra_can_be_sfx           COMMAND test_lib_bra test_lib_bra_can_be_sfx)

#####################################################################################################

add_executable(test_bra_tree_dir test_bra_tree_dir.cpp)
target_link_libraries(test_bra_tree_dir PRIVATE lib_bra)

add_test(NAME test_bra_tree_dir.test_bra_tree_dir_add1           COMMAND test_bra_tree_dir test_bra_tree_dir_add1)
add_test(NAME test_bra_tree_dir.test_bra_tree_dir_add2           COMMAND test_bra_tree_dir test_bra_tree_dir_add2)
add_test(NAME test_bra_tree_dir.test_bra_tree_dir_add3           COMMAND test_bra_tree_dir test_bra_tree_dir_add3)

#####################################################################################################


add_executable(test_bra_wildcards test_bra_wildcards.cpp)
target_link_libraries(test_bra_wildcards PRIVATE lib_bra)

add_test(NAME test_bra_wildcards.is_wildcard           COMMAND test_bra_wildcards test_bra_wildcards_is_wildcard)
add_test(NAME test_bra_wildcards.wildcard_extract_dir  COMMAND test_bra_wildcards test_bra_wildcards_extract_dir)
add_test(NAME test_bra_wildcards.wildcard_to_regexp    COMMAND test_bra_wildcards test_bra_wildcards_to_regexp)

#####################################################################################################


add_executable(test_bra_fs test_bra_fs.cpp)
target_link_libraries(test_bra_fs PRIVATE lib_bra)


add_test(NAME test_bra_fs.file_exists           COMMAND test_bra_fs test_bra_fs_file_exists)
add_test(NAME test_bra_fs.dir_exists            COMMAND test_bra_fs test_bra_fs_dir_exists)

add_test(NAME test_bra_fs.try_sanitize_path     COMMAND test_bra_fs test_bra_fs_try_sanitize_path)
add_test(NAME test_bra_fs.dir_make              COMMAND test_bra_fs test_bra_fs_dir_make)
add_test(NAME test_bra_fs.dir_isSubDir          COMMAND test_bra_fs test_bra_fs_dir_isSubDir)

add_test(NAME test_bra_fs.sfx_filename_adjust               COMMAND test_bra_fs test_bra_fs_sfx_filename_adjust)
add_test(NAME test_bra_fs.search_wildcard                   COMMAND test_bra_fs test_bra_fs_search_wildcard)
add_test(NAME test_bra_fs.search_wildcard_recursive_on_dir1 COMMAND test_bra_fs test_bra_fs_search_wildcard_recursive_on_dir1)
add_test(NAME test_bra_fs.search_wildcard_recursive_in_dir1 COMMAND test_bra_fs test_bra_fs_search_wildcard_recursive_in_dir1)

add_test(NAME test_bra_fs.file_attributes COMMAND test_bra_fs test_bra_fs_file_attributes)


# add_test(NAME test_bra_fs.symlink COMMAND test_bra_fs test_bra_fs_symlink)

#####################################################################################################

add_executable(test_bra test_bra.cpp)
target_link_libraries(test_bra PRIVATE lib_bra)
add_dependencies(test_bra bra unbra bra_sfx)

add_test(NAME test_bra.help_ret_code                 COMMAND test_bra test_bra_help_ret_code)
add_test(NAME test_bra.no_output_file                COMMAND test_bra test_bra_no_output_file)

add_test(NAME test_bra.unbra                         COMMAND test_bra test_bra_unbra)
add_test(NAME test_bra.test_bra_unbra_list_recursive COMMAND test_bra test_bra_unbra_list_recursive)

add_test(NAME test_bra.dir_no_wildcard               COMMAND test_bra test_bra_dir_no_wildcard)
add_test(NAME test_bra.wildcard_dir_unbra_list       COMMAND test_bra test_bra_wildcard_dir_unbra_list)

add_test(NAME test_bra.sfx_0                         COMMAND test_bra test_bra_sfx_0)
add_test(NAME test_bra.sfx_1                         COMMAND test_bra test_bra_sfx_1)
add_test(NAME test_bra.sfx_2                         COMMAND test_bra test_bra_sfx_2)

add_test(NAME test_bra.not_more_than_1_same_file     COMMAND test_bra test_bra_not_more_than_1_same_file)

add_test(NAME test_bra.unbra_all                     COMMAND test_bra test_bra_unbra_all)

#####################################################################################################

add_executable(test_bra_encoders test_bra_encoders.cpp)
target_link_libraries(test_bra_encoders PUBLIC lib_bra)

add_test(NAME test_bra_encoders.encode_decode_rle_1  COMMAND test_bra_encoders test_bra_encoders_encode_decode_rle_1)
add_test(NAME test_bra_encoders.encode_decode_rle_2  COMMAND test_bra_encoders test_bra_encoders_encode_decode_rle_2)
add_test(NAME test_bra_encoders.encode_decode_rle_3  COMMAND test_bra_encoders test_bra_encoders_encode_decode_rle_3)
add_test(NAME test_bra_encoders.encode_decode_rle_3b COMMAND test_bra_encoders test_bra_encoders_encode_decode_rle_3b)

#####################################################################################################


# set_tests_properties(
#     test_bra.no_output_file
#     test_bra.unbra
#     test_bra.sfx_0
#     test_bra.sfx_1
#     test_bra.sfx_2
#     test_bra.not_more_than_1_same_file
#     PROPERTIES WORKING_DIRECTORY
#     ${CMAKE_CURRENT_BINARY_DIR}
# )

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

add_custom_command(TARGET test_bra POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dir1
                    ${CMAKE_CURRENT_BINARY_DIR}/dir1
)

foreach(fixture_i IN ITEMS dir1)
add_custom_command(TARGET test_bra_fs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/${fixture_i}
        ${CMAKE_CURRENT_BINARY_DIR}/${fixture_i}
)
endforeach()
