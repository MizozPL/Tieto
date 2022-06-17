#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "Watchdog.h"
#include "Logger.h"

static struct timespec WATCHDOG_UPDATE_INTERVAL = {.tv_sec = 2, .tv_nsec = 0};

typedef struct Watch {
    void (*function)(void *);

    void *object;
    bool status;
} Watch;

struct Watchdog {
    size_t watches;
    size_t registered_count;
    pthread_t thread;
    pthread_mutex_t mutex;
    bool watching;
    bool should_stop;
    Watch watches_array[];
};

static bool watchdog_should_stop_synchronized(Watchdog *watchdog);

static void *watchdog_thread(void *args);

Watchdog *watchdog_create(size_t watches) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_create: Entry.");

    if (watches <= 0) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received watchdog_create call with watches <= 0.");
        return NULL;
    }

    Watchdog *watchdog = malloc(sizeof(Watchdog) + sizeof(Watch) * watches);
    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received NULL from malloc call in watchdog_create.");
        return NULL;
    }

    *watchdog = (Watchdog) {
            .watches = watches,
            .registered_count = 0,
            .watching = false,
            .should_stop = false,
            .mutex = PTHREAD_MUTEX_INITIALIZER
    };

    if (pthread_create(&watchdog->thread, NULL, watchdog_thread, (void *) watchdog) != 0) {
        logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received error from pthread_create in watchdog_create.");
        pthread_mutex_destroy(&watchdog->mutex);
        free(watchdog);
        return NULL;
    }

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_create: Success.");
    return watchdog;
}

void watchdog_await_and_destroy(Watchdog *watchdog) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_await_and_destroy: Entry.");

    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received watchdog_await_and_destroy call with watchdog = NULL.");
        return;
    }

    pthread_join(watchdog->thread, NULL);
    pthread_mutex_destroy(&watchdog->mutex);
    free(watchdog);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_await_and_destroy: Success.");
}

void watchdog_request_stop_synchronized(Watchdog *watchdog) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_request_stop_synchronized: Entry.");

    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received watchdog_request_stop_synchronized call with watchdog = NULL.");
        return;
    }

    pthread_mutex_lock(&watchdog->mutex);
    watchdog->should_stop = true;
    pthread_mutex_unlock(&watchdog->mutex);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_request_stop_synchronized: Success.");
}

size_t watchdog_register_watch(Watchdog *watchdog, void (*function)(void *), void *object) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_register_watch: Entry.");

    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received watchdog_register_watch call with watchdog = NULL.");
        return 0;
    }

    size_t return_value;
    pthread_mutex_lock(&watchdog->mutex);
    return_value = watchdog->registered_count++;
    watchdog->watches_array[return_value] = (Watch) {
            .status = true,
            .function = function,
            .object = object
    };
    pthread_mutex_unlock(&watchdog->mutex);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_register_watch: Success.");
    return return_value;
}

void watchdog_update(Watchdog *watchdog, size_t index) {
    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received watchdog_update call with watchdog = NULL.");
        return;
    }

    if (index >= watchdog->watches) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received watchdog_update call with watches index >= watches.");
        return;
    }

    pthread_mutex_lock(&watchdog->mutex);
    watchdog->watches_array[index].status = true;
    pthread_mutex_unlock(&watchdog->mutex);
}

void watchdog_start_watching(Watchdog *watchdog) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_start_watching: Entry.");

    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received watchdog_start_watching call with watchdog = NULL.");
        return;
    }

    pthread_mutex_lock(&watchdog->mutex);
    watchdog->watching = true;
    pthread_mutex_unlock(&watchdog->mutex);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_start_watching: Success.");
}

void watchdog_pause_watching(Watchdog *watchdog) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_pause_watching: Entry.");

    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received watchdog_pause_watching call with watchdog = NULL.");
        return;
    }

    pthread_mutex_lock(&watchdog->mutex);
    watchdog->watching = false;
    pthread_mutex_unlock(&watchdog->mutex);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_pause_watching: Success.");
}

static bool watchdog_should_stop_synchronized(Watchdog *watchdog) {
    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received watchdog_should_stop_synchronized call with watchdog = NULL.");
        return true;
    }

    bool return_value;
    pthread_mutex_lock(&watchdog->mutex);
    return_value = watchdog->should_stop;
    pthread_mutex_unlock(&watchdog->mutex);
    return return_value;
}

static void *watchdog_thread(void *args) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_thread: Entry.");
    Watchdog *watchdog = (Watchdog *) args;

    while (!watchdog_should_stop_synchronized(watchdog)) {
        logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_thread: Iteration.");
        bool flag = false;
        pthread_mutex_lock(&watchdog->mutex);
        for (size_t i = 0; i < watchdog->watches; i++) {
            if (watchdog->watching && !watchdog->watches_array[i].status) {
                flag = true;
            }
            watchdog->watches_array[i].status = false;
        }
        pthread_mutex_unlock(&watchdog->mutex);

        if (flag) {
            logger_log(logger_get_global(), LOGGER_LEVEL_WARN, "watchdog_thread: flagged. Stopping program.");
            pthread_mutex_lock(&watchdog->mutex);
            watchdog->should_stop = true;
            for (size_t i = 0; i < watchdog->watches; i++) {
                watchdog->watches_array[i].function(watchdog->watches_array[i].object);
            }
            pthread_mutex_unlock(&watchdog->mutex);
        } else {
            nanosleep(&WATCHDOG_UPDATE_INTERVAL, NULL);
        }
    }

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "watchdog_thread: Ending.");
    return NULL;
}

