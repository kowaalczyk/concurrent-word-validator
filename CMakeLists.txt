cmake_minimum_required(VERSION 3.8)
project(pw_validator)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-g -Wall")


add_library(err STATIC src/err.h src/err.c) # TODO: Add custom err library

# libraries
add_library(config STATIC src/config.c src/config.h)

add_library(automaton STATIC src/automaton.h src/automaton.c)
target_link_libraries(automaton config err)

add_library(validator_mq STATIC src/validator_mq.c src/validator_mq.h)
target_link_libraries(validator_mq config err rt)

add_library(tester_mq STATIC src/tester_mq.c src/tester_mq.h)
target_link_libraries(tester_mq config err rt)


# http://stackoverflow.com/questions/10555706/
#macro (add_executable _name)
#    # invoke built-in add_executable
#    _add_executable(${ARGV})
#    if (TARGET ${_name})
#        target_link_libraries(${_name} err rt m config automaton validator_mq tester_mq)
#    endif()
#endmacro()

# release executables
add_executable(validator src/validator.c)
target_link_libraries(validator config automaton validator_mq tester_mq)

add_executable(tester src/tester.c)
target_link_libraries(tester config validator_mq tester_mq)

add_executable(run src/run.c)
target_link_libraries(run config validator_mq automaton)

# test executables
add_executable(struct_load_malloc_test test/struct_memory_alloc_test.c) # TODO: Fix refactored names
target_link_libraries(struct_load_malloc_test config automaton)

add_executable(seq_word_validation_test test/seq_word_validation_test.c) # TODO: Fix refactored names
target_link_libraries(seq_word_validation_test config automaton)

add_executable(tester_mq_test test/tester_mq_test.c)
target_link_libraries(tester_mq_test config tester_mq)

add_executable(validator_mq_test test/validator_mq_test.c)
target_link_libraries(validator_mq_test config validator_mq)
# TODO: Automaton pipe send test

# sandbox for playing around with MQs
add_executable(mq_sandbox test/mq_sandbox.c)
target_link_libraries(mq_sandbox rt config err)

# pipe large struct test: compile child first, run parent and diff their output files (shold be the same)
add_executable(pipe_cpy_test_child test/pipe_cpy_test_child.c)
add_executable(pipe_cpy_test test/pipe_cpy_test.c)


install(TARGETS DESTINATION .)
