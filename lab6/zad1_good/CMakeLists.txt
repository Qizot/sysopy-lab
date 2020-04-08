cmake_minimum_required(VERSION 2.8.12)
project(server)
project(client)

set(CMAKE_C_STANDARD 99)
add_executable(server server.c chat.c)
add_executable(client client.c  chat.c)
