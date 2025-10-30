# List of tests
set(TESTS
        hash_test
        rb_tree_test
        dyn_array_test
        string_builder_test
)

foreach(test_name IN LISTS TESTS)
    add_executable(${test_name} ${test_name}.c)
    target_link_libraries(${test_name} PRIVATE gc_lib)

    # Copy assets folder after building the test
    add_custom_command(TARGET ${test_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${test_name}>/assets
    )

    # Register test with ctest
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
