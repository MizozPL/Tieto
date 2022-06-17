cmake_minimum_required(VERSION 3.16.3)
project(Tieto C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_compile_options(-Wall -Wextra -pedantic -Werror -Weverything -Wno-disabled-macro-expansion -Wno-padded)
add_executable(Tieto src/main.c src/Reader.c src/Reader.h src/Queue.c src/Queue.h src/Analyzer.c src/Analyzer.h src/LongDoubleArray.c src/LongDoubleArray.h src/Printer.c src/Printer.h src/Watchdog.c src/Watchdog.h src/Logger.c src/Logger.h)
target_link_libraries(Tieto Threads::Threads)