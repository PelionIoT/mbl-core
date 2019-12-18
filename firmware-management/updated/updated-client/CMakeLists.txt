cmake_minimum_required(VERSION 3.5)

include(GNUInstallDirs)

set(UPDATED_CLIENT_SRC
    source/rpc/Client.cpp
    source/updated-client.cpp
)

add_executable(updated-client ${UPDATED_CLIENT_SRC})
target_link_libraries(updated-client common_compile_options)
target_link_libraries(updated-client common_compile_warnings)
target_link_libraries(updated-client updated-rpc)

install(TARGETS updated-client DESTINATION ${CMAKE_INSTALL_BINDIR})

