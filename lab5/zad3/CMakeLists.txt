cmake_minimum_required(VERSION 3.15)
project(zad2 C)

set(CMAKE_C_STANDARD 11)

add_executable(main main.c)
add_executable(producer producer.c)
add_executable(consumer consumer.c)