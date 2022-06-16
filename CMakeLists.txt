cmake_minimum_required(VERSION 3.16.3)
project(Tieto C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(Tieto main.c Reader.c Reader.h)
target_link_libraries(Tieto Threads::Threads)