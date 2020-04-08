cmake_minimum_required(VERSION 3.15)
project(zad1)

set(CMAKE_C_STANDARD 99)



add_executable(client client.c chat_utils.c)
add_executable(server server.c chat_utils.c)
