cmake_minimum_required(VERSION 3.16)
project(udp C)

set(CMAKE_C_STANDARD 11)

add_executable(server  server.c common.c)
add_executable(client client.c common.c)

target_link_libraries(server -lpthread -lrt)
target_link_libraries(client -lpthread -lrt)
