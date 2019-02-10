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
    ${CMAKE_SOURCE_DIR}/src/DBusAdapterTests.cpp
    ${CMAKE_SOURCE_DIR}/src/TestInfraAppThread.cpp
    ${CMAKE_SOURCE_DIR}/src/TestInfra_DBusAdapterTester.cpp

    ${CMAKE_SOURCE_DIR}/src/DBusAdapter.cpp
    ${CMAKE_SOURCE_DIR}/src/DBusAdapter_Impl.cpp

    ${CMAKE_SOURCE_DIR}/src/Mailbox.cpp
    ${CMAKE_SOURCE_DIR}/src/SelfEvent.cpp      
    ${CMAKE_SOURCE_DIR}/src/EventManager.cpp
    ${CMAKE_SOURCE_DIR}/src/DBusService.c
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
                    
