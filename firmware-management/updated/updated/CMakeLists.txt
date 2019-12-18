cmake_minimum_required(VERSION 3.5)

include(GNUInstallDirs)

set(UPDATED_SRC
    source/init.cpp
    source/updated.cpp

    source/rpc/Server.cpp
    source/rpc/ServiceImpl.cpp
)

add_executable(updated ${UPDATED_SRC})
target_link_libraries(updated common_compile_options)
target_link_libraries(updated common_compile_warnings)
target_link_libraries(updated updated-rpc)
target_link_libraries(updated systemd)

install(TARGETS updated DESTINATION ${CMAKE_INSTALL_BINDIR})

