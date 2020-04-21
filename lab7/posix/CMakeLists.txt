cmake_minimum_required(VERSION 3.16)
project(posix C)

#set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
#find_package(Threads REQUIRED)
#if(CMAKE_USE_PTHREADS_INIT)
#    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")
#endif()
set(CMAKE_C_STANDARD 11)

add_executable(shop shop.c delivery_shop.c utils.h)

add_executable(worker worker.c delivery_shop.c utils.h)

target_link_libraries(shop -lpthread -lrt)
target_link_libraries(worker -lpthread -lrt)
