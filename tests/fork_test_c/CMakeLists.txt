add_executable(fork_test_c "fork_test.c")
target_link_libraries(fork_test_c PRIVATE forklib)