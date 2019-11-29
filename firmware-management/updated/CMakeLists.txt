project(UpdateD)

cmake_minimum_required(VERSION 3.5)

add_executable(updated ${CMAKE_CURRENT_SOURCE_DIR}/updated.cpp)

target_compile_options(updated PUBLIC -Wall -Werror -Wextra -std=c++17)

install(TARGETS updated DESTINATION bin)

