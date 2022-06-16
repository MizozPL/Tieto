//
// Created by Admin on 16.06.2022.
//

#ifndef TIETO_WATCHDOG_H
#define TIETO_WATCHDOG_H

typedef struct Watchdog Watchdog;

void watchdog_request_stop_synchronized(Watchdog* watchdog);
size_t watchdog_register_watch(Watchdog* watchdog, void (*function)(void*), void *object);
void watchdog_update(Watchdog* watchdog, size_t index);
Watchdog *watchdog_create(size_t watches);
void watchdog_start_watching(Watchdog *watchdog);
void watchdog_stop_watching(Watchdog* watchdog);
void watchdog_await_and_destroy(Watchdog *watchdog);

#endif //TIETO_WATCHDOG_H
