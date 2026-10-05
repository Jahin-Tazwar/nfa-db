cmake_minimum_required(VERSION 3.15)

project(NFA-DB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pager library
add_library(pager
    src/pager.cpp
)

target_include_directories(pager PUBLIC
    include
)

# Main program
add_executable(nfa_db
    src/main.cpp
)

target_link_libraries(nfa_db PRIVATE
    pager
)

# Tests
add_executable(pager_test
    tests/pager_test.cpp
)

target_link_libraries(pager_test PRIVATE
    pager
)

enable_testing()

add_test(
    NAME pager_test
    COMMAND pager_test
)