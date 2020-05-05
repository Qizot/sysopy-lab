cmake_minimum_required(VERSION 3.16)
project(lab9 C)

set(CMAKE_C_STANDARD 11)

add_executable(lab9 main.c)
target_link_libraries(lab9 -lpthread -lrt)