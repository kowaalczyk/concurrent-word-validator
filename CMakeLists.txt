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
add_library(automaton STATIC src/automaton.h src/automaton.c)
add_library(validator_queues STATIC src/validator_queues.c src/validator_queues.h)

# release executables
add_executable(validator src/validator.c)
add_executable(tester src/tester.c)
add_executable(run src/run.c)

# test executables
add_executable(struct_load_malloc_test test/struct_memory_alloc_test.c)
add_executable(seq_word_validation_test test/seq_word_validation_test.c)
