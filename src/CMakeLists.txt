add_library(Analyzer Analyzer.c)
target_include_directories(Analyzer PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_library(Logger Logger.c)
target_include_directories(Logger PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_library(LongDoubleArray LongDoubleArray.c)
target_include_directories(LongDoubleArray PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_library(Printer Printer.c)
target_include_directories(Printer PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_library(Queue Queue.c)
target_include_directories(Queue PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_library(Reader Reader.c)
target_include_directories(Reader PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_library(Watchdog Watchdog.c)
target_include_directories(Watchdog PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_executable(Tieto main.c)
target_link_libraries(Tieto Analyzer Logger LongDoubleArray Printer Queue Reader Watchdog)
target_link_libraries(Tieto Threads::Threads)