cmake_minimum_required(VERSION 2.6)

# Locate GTest
find_package(GTest REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD systemd)

cmake_minimum_required (VERSION 2.8)
SET(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(CMAKE_BUILD_TYPE Debug)

# Add these options after all non-mbl-cloud-client bits so they only apply to
# mbl-cloud-client code
add_compile_options(-Werror)
add_compile_options(-Wall)
add_compile_options(-Wextra)
add_compile_options(-Wformat=2)
add_compile_options(-Wunused-parameter)
add_compile_options(-Wfloat-equal)
add_compile_options(-Wshadow)
add_compile_options(-Wcast-qual)
add_compile_options(-Wcast-align)
add_compile_options(-Wwrite-strings)
add_compile_options(-Wconversion)
add_compile_options(-Wlogical-op)

include_directories(
    ${GTEST_INCLUDE_DIRS}
    ${SYSTEMD_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR})

message(STATUS "dirs: ${SYSTEMD_INCLUDE_DIRS}")

FILE(GLOB MyCSources
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/gtest/*.cpp"
)

# Link runTests with what we want to test and the GTest and pthread library
add_executable(runTests ${MyCSources})
    
set_target_properties (runTests PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED TRUE
    CXX_EXTENSIONS FALSE
  )

target_link_libraries(runTests 
    ${GTEST_LIBRARIES} 
    systemd
    pthread)
                    
