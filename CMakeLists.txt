cmake_minimum_required(VERSION 3.8)
project(pw_validator)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-g -Wall")

# tp://stackoverflow.com/questions/10555706/
macro (add_executable _name)
    # invoke built-in add_executable
    _add_executable(${ARGV})
    if (TARGET ${_name})
        target_link_libraries(${_name} err)
    endif()
endmacro()

add_library(err src/err.c)

add_executable(validator src/validator.c)
add_executable(tester src/tester.c)
add_executable(run src/run.c)