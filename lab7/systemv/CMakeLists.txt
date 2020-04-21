cmake_minimum_required(VERSION 3.16)
project(systemv C)

set(CMAKE_C_STANDARD 11)

add_executable(shop shop.c delivery_shop.c utils.h)

add_executable(worker worker.c delivery_shop.c utils.h)