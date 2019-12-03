project(UpdateD)

cmake_minimum_required(VERSION 3.5)

add_executable(updated ${CMAKE_CURRENT_SOURCE_DIR}/updated.cpp)

target_compile_options(updated
    PUBLIC
    -Wall
    -Werror
    -Wextra
    -std=c++17
    -Wformat=2
    -Wunused-parameter
    -Wfloat-equal
    -Wshadow
    -Wcast-qual
    -Wcast-align
    -Wwrite-strings
    -Wconversion
    -Wlogical-op)


install(TARGETS updated DESTINATION bin)
