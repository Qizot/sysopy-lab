cmake_minimum_required(VERSION 3.15)
project(lab2 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(lab2
        lib.c
        lib.h
        main.c)
