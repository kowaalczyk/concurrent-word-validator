cmake_minimum_required(VERSION 3.8)
project(pw_validator)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE  "-Wall -Wextra -Werror -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE  "-Wall -Wextra -Werror -O3 -DNDEBUG")


# libraries
add_library(config STATIC src/config.c src/config.h)

add_library(automaton STATIC src/automaton.h src/automaton.c)
target_link_libraries(automaton config)

add_library(tester_list STATIC src/tester_list.h src/tester_list.c)
target_link_libraries(tester_list config)

add_library(pid_list STATIC src/pid_list.h src/pid_list.c)
target_link_libraries(pid_list config)

add_library(validator_mq STATIC src/validator_mq.c src/validator_mq.h)
target_link_libraries(validator_mq config rt)

add_library(tester_mq STATIC src/tester_mq.c src/tester_mq.h)
target_link_libraries(tester_mq config rt)


# release executables
add_executable(validator src/validator.c)
target_link_libraries(validator config automaton validator_mq tester_mq tester_list pid_list)

add_executable(tester src/tester.c)
target_link_libraries(tester config validator_mq tester_mq)

add_executable(run src/run.c)
target_link_libraries(run config validator_mq automaton)


# test executables
add_executable(seq_word_validation_test test/seq_word_validation_test.c)
target_link_libraries(seq_word_validation_test config automaton)

add_executable(tester_mq_test test/tester_mq_test.c)
target_link_libraries(tester_mq_test config tester_mq)

add_executable(validator_mq_test test/validator_mq_test.c)
target_link_libraries(validator_mq_test config validator_mq)

add_executable(exec_wait_return_test test/exec_wait_return_test.c)
add_executable(casting_to_argv_test test/casting_to_argv_test.c)

add_executable(async_func_sig_return_test test/async_func_sig_return_test.c)
target_link_libraries(async_func_sig_return_test config)

add_executable(fork_reader_sender_test test/fork_reader_sender_test.c)

add_executable(parallel_word_validation_test test/parallel_word_validation_test.c)
target_link_libraries(parallel_word_validation_test config automaton rt)

install(TARGETS DESTINATION .)
