cmake_minimum_required(VERSION 3.8)
project(pw_validator)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-g -Wall")

# http://stackoverflow.com/questions/10555706/
macro (add_executable _name)
    # invoke built-in add_executable
    _add_executable(${ARGV})
    if (TARGET ${_name})
        target_link_libraries(${_name} err rt m)
    endif()
endmacro()

add_library(err STATIC src/err.h src/err.c)

add_library(config STATIC src/config.c src/config.h)
add_library(automaton STATIC src/automaton.h src/automaton.c)
add_library(validator_mq STATIC src/validator_mq.c src/validator_mq.h)
add_library(tester_mq STATIC src/tester_mq.c src/tester_mq.h)

# release executables
add_executable(validator src/validator.c)
add_executable(tester src/tester.c)
add_executable(run src/run.c)

# test executables
add_executable(struct_load_malloc_test test/struct_memory_alloc_test.c)
add_executable(seq_word_validation_test test/seq_word_validation_test.c)

# pipe large struct test: compile child first, run parent and diff their output files (shold be the same)
add_executable(pipe_cpy_test_child test/pipe_cpy_test_child.c)
add_executable(pipe_cpy_test test/pipe_cpy_test.c)
