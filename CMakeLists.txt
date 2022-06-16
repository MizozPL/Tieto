cmake_minimum_required(VERSION 3.16.3)
project(Tieto C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_compile_options(-Wall -Wextra -pedantic -Werror -Weverything -Wno-disabled-macro-expansion)
add_executable(Tieto main.c Reader.c Reader.h Queue.c Queue.h Analyzer.c Analyzer.h LongDoubleArray.c LongDoubleArray.h Printer.c Printer.h Watchdog.c Watchdog.h cmake-build-debug/Logger.c cmake-build-debug/Logger.h)
target_link_libraries(Tieto Threads::Threads)