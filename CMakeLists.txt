cmake_minimum_required(VERSION 3.16.3)
project(Tieto C)

#set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_C_STANDARD 99)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if(CMAKE_C_COMPILER_ID MATCHES GNU)
    add_compile_options(-pedantic -Wall -Wextra -Wno-disabled-macro-expansion -Wno-padded)
endif()

if(CMAKE_C_COMPILER_ID MATCHES Clang)
    add_compile_options(-pedantic -Weverything -Wno-disabled-macro-expansion -Wno-padded)
endif()


add_executable(Tieto src/main.c src/Reader.c include/Reader.h src/Queue.c include/Queue.h src/Analyzer.c include/Analyzer.h src/LongDoubleArray.c include/LongDoubleArray.h src/Printer.c include/Printer.h src/Watchdog.c include/Watchdog.h src/Logger.c include/Logger.h)
target_link_libraries(Tieto Threads::Threads)