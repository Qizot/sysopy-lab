cmake_minimum_required(VERSION 3.15)
project(lab1 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(lab1
    library.c
    library.h
    main.c)
