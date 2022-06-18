#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include "../include/Watchdog.h"
#include "../include/Logger.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int objectA = 2;
static int objectB = 8;
static bool flagA = false;
static bool flagB = false;

void stopA(void *object) {
    assert(&objectA == object);

    pthread_mutex_lock(&mutex);
    flagA = true;
    pthread_mutex_unlock(&mutex);
}

void stopB(void *object) {
    assert(&objectB == object);

    pthread_mutex_lock(&mutex);
    flagB = true;
    pthread_mutex_unlock(&mutex);
}

int main(void) {
    Watchdog *watchdog = watchdog_create(2);
    size_t a = watchdog_register_watch(watchdog, &stopA, &objectA);
    size_t b = watchdog_register_watch(watchdog, &stopB, &objectB);
    watchdog_update(watchdog, a);
    watchdog_update(watchdog, b);
    watchdog_start_watching(watchdog);

    sleep(1);
    watchdog_update(watchdog, a);
    watchdog_update(watchdog, b);
    sleep(1);
    watchdog_update(watchdog, a);
    watchdog_update(watchdog, b);
    sleep(1);
    watchdog_update(watchdog, a);
    watchdog_update(watchdog, b);
    sleep(1);
    watchdog_update(watchdog, a);
    watchdog_update(watchdog, b);
    sleep(1);
    watchdog_update(watchdog, a);
    watchdog_update(watchdog, b);
    sleep(1);
    watchdog_update(watchdog, a);
    watchdog_update(watchdog, b);

    pthread_mutex_lock(&mutex);
    assert(!flagA && !flagB);
    pthread_mutex_unlock(&mutex);

    sleep(1);
    watchdog_update(watchdog, a);
    watchdog_update(watchdog, b);

    sleep(4);
    pthread_mutex_lock(&mutex);
    assert(flagA && flagB);
    pthread_mutex_unlock(&mutex);

    watchdog_request_stop_synchronized(watchdog);
    watchdog_await_and_destroy(watchdog);

    pthread_mutex_destroy(&mutex);

    logger_destroy(logger_get_global());

    return 0;
}
