project(UpdateD)

cmake_minimum_required(VERSION 3.5)

option(RUN_CODE_CHECKS OFF)

if(RUN_CODE_CHECKS)
    set(CMAKE_CXX_CLANG_TIDY clang-tidy;-checks=*,-clang-analyzer-cplusplus-*,clang-analyzer-*)
endif(RUN_CODE_CHECKS)

add_executable(updated ${CMAKE_CURRENT_SOURCE_DIR}/updated.cpp ${CMAKE_CURRENT_SOURCE_DIR}/init.cpp)
target_include_directories(updated PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

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
    "$<$<CXX_COMPILER_ID:GNU>:-Wlogical-op>")

target_link_libraries(updated systemd)

install(TARGETS updated RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
