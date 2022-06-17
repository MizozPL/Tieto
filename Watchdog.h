#ifndef TIETO_WATCHDOG_H
#define TIETO_WATCHDOG_H

typedef struct Watchdog Watchdog;

Watchdog *watchdog_create(size_t watches);

void watchdog_await_and_destroy(Watchdog *watchdog);

void watchdog_request_stop_synchronized(Watchdog *watchdog);

size_t watchdog_register_watch(Watchdog *watchdog, void (*function)(void *), void *object);

void watchdog_update(Watchdog *watchdog, size_t index);

void watchdog_start_watching(Watchdog *watchdog);

void watchdog_pause_watching(Watchdog *watchdog);

#endif //TIETO_WATCHDOG_H
