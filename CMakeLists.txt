cmake_minimum_required(VERSION 3.15)

project(NFA-DB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pager library
add_library(pager
    src/pager.cpp
)

add_library(record
    src/record.cpp
)

target_include_directories(pager PUBLIC
    include
)

target_include_directories(record PUBLIC
    include
)

# Main program
add_executable(nfa_db
    src/main.cpp
)

target_link_libraries(nfa_db PRIVATE
    pager
    record
)

# Tests
add_executable(pager_test
    tests/pager_test.cpp
)

add_executable(record_test
    tests/record_test.cpp
)

target_link_libraries(pager_test PRIVATE
    pager
)

target_link_libraries(record_test PRIVATE
    record
)

enable_testing()

add_test(
    NAME pager_test
    COMMAND pager_test
)

add_test(
    NAME record_test
    COMMAND record_test
)