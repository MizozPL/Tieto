//
// Created by Admin on 16.06.2022.
//

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "Watchdog.h"

static struct timespec WATCHDOG_UPDATE_INTERVAL = { .tv_sec = 2, .tv_nsec = 0 };

typedef struct Watch {
    void (*function)(void*);
    void *object;
    char padding[7]; //That's just stupid!
    bool status;
} Watch;

struct Watchdog {
    size_t watches;
    size_t registered_count;
    pthread_t thread;
    pthread_mutex_t mutex;
    bool watching;
    bool should_stop;
    char padding[6]; //That's just stupid!
    Watch watches_array[];
};

void watchdog_request_stop_synchronized(Watchdog* watchdog) {
    if(watchdog == NULL) {
        perror("watchdog_request_stop_synchronized called on NULL");
        return;
    }

    pthread_mutex_lock(&watchdog->mutex);
    watchdog->should_stop = true;
    pthread_mutex_unlock(&watchdog->mutex);
}

static bool watchdog_should_stop_synchronized(Watchdog* watchdog) {
    if(watchdog == NULL) {
        perror("watchdog_should_stop_synchronized called on NULL");
        return true;
    }

    bool return_value;
    pthread_mutex_lock(&watchdog->mutex);
    return_value = watchdog->should_stop;
    pthread_mutex_unlock(&watchdog->mutex);
    return return_value;
}

static void *watchdog_thread(void* args) {
    Watchdog* watchdog = (Watchdog *) args;
    while(!watchdog_should_stop_synchronized(watchdog)){
        bool flag = false;
        pthread_mutex_lock(&watchdog->mutex);
        for(size_t i = 0; i < watchdog->watches; i++) {
            if(watchdog->watching && !watchdog->watches_array[i].status) {
                flag = true;
            }
            watchdog->watches_array[i].status = false;
        }
        pthread_mutex_unlock(&watchdog->mutex);

        if(flag) {
            pthread_mutex_lock(&watchdog->mutex);
            watchdog->should_stop = true;
            for(size_t i = 0; i < watchdog->watches; i++) {
                watchdog->watches_array[i].function(watchdog->watches_array[i].object);
            }
            pthread_mutex_unlock(&watchdog->mutex);
        } else {
            nanosleep(&WATCHDOG_UPDATE_INTERVAL, NULL);
        }
    }

    return NULL;
}

Watchdog *watchdog_create(size_t watches) {
    if(watches <= 0) {
        perror("watchdog_create watches <= 0");
        return NULL;
    }

    Watchdog* watchdog = malloc(sizeof(Watchdog) + sizeof(Watch) * watches);
    if(watchdog == NULL) {
        perror("malloc error");
        return NULL;
    }

    *watchdog = (Watchdog) {
        .watches = watches,
        .registered_count = 0,
        .watching = false,
        .should_stop = false,
        .mutex = PTHREAD_MUTEX_INITIALIZER
    };

    if(pthread_create(&watchdog->thread, NULL, watchdog_thread, (void*) watchdog) != 0) {
        perror("pthread_create error");
        pthread_mutex_destroy(&watchdog->mutex);
        free(watchdog);
        return NULL;
    }

    return watchdog;
}

size_t watchdog_register_watch(Watchdog* watchdog, void (*function)(void*), void *object) {
    if(watchdog == NULL) {
        perror("watchdog_register_watch called on NULL");
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
    return return_value;
}

void watchdog_update(Watchdog* watchdog, size_t index) {
    if(watchdog == NULL) {
        perror("watchdog_update called on NULL");
        return;
    }

    if(index >= watchdog->watches) {
        perror("watchdog_update index >= watchdog->watches");
        return;
    }

    pthread_mutex_lock(&watchdog->mutex);
    watchdog->watches_array[index].status = true;
    pthread_mutex_unlock(&watchdog->mutex);
}

void watchdog_start_watching(Watchdog* watchdog) {
    if(watchdog == NULL) {
        perror("watchdog_start_watching called on NULL");
        return;
    }

    pthread_mutex_lock(&watchdog->mutex);
    watchdog->watching = true;
    pthread_mutex_unlock(&watchdog->mutex);
}

void watchdog_stop_watching(Watchdog* watchdog) {
    if(watchdog == NULL) {
        perror("watchdog_stop_watching called on NULL");
        return;
    }

    pthread_mutex_lock(&watchdog->mutex);
    watchdog->watching = false;
    pthread_mutex_unlock(&watchdog->mutex);
}

void watchdog_await_and_destroy(Watchdog* watchdog) {
    if(watchdog == NULL) {
        return;
    }

    pthread_join(watchdog->thread, NULL);
    pthread_mutex_destroy(&watchdog->mutex);
    free(watchdog);
}

