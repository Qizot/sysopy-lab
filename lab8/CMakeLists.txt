cmake_minimum_required(VERSION 3.16)
project(lab8 C)

set(CMAKE_C_STANDARD 11)

add_executable(lab8 main.c)
target_link_libraries(lab8 -lpthread -lrt -lm)