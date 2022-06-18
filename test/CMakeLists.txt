add_executable(WatchdogTest WatchdogTest.c)
target_link_libraries(WatchdogTest Watchdog Logger)
target_link_libraries(WatchdogTest Threads::Threads)

add_executable(QueueTest QueueTest.c)
target_link_libraries(QueueTest Queue Logger)