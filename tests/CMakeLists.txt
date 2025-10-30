add_executable(test_hello test_hello.c)
target_link_libraries(test_hello PRIVATE gc_lib)
add_test(NAME test_hello COMMAND test_hello)
