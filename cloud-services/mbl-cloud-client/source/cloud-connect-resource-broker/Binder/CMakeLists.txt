cmake_minimum_required(VERSION 2.6)

# Locate GTest
find_package(GTest REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD systemd)

set(CMAKE_BUILD_TYPE Debug)

include_directories(
    ${GTEST_INCLUDE_DIRS}
    ${SYSTEMD_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/inc)

message(STATUS "dirs: ${SYSTEMD_INCLUDE_DIRS}")

# Link runTests with what we want to test and the GTest and pthread library
add_executable(runTests 
    ${CMAKE_SOURCE_DIR}/src/tests.cpp 
    ${CMAKE_SOURCE_DIR}/src/DBusAdapter.cpp
    #${CMAKE_SOURCE_DIR}/src/DBusAdapterMailbox.cpp
    #${CMAKE_SOURCE_DIR}/src/DBusAdapterTimer.cpp    
    #${CMAKE_SOURCE_DIR}/src/DBusAdapterLowLevel.c     
    ${CMAKE_SOURCE_DIR}/src/DBusAdapterService.c
    )
    
set_target_properties (runTests PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED TRUE
    CXX_EXTENSIONS FALSE
  )

target_link_libraries(runTests 
    ${GTEST_LIBRARIES} 
    systemd
    pthread)
                    
