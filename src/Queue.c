#include "../include/Queue.h"
#include "../include/Logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

struct Queue {
    size_t capacity;
    size_t size;
    size_t head;
    size_t tail;
    pthread_mutex_t mutex;
    pthread_cond_t can_insert;
    pthread_cond_t can_extract;
    void *buffer[];
};

Queue *queue_create(const size_t capacity) {
    if (capacity <= 0) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_create call with capacity <= 0.");
        return NULL;
    }

    Queue *queue = malloc(sizeof(Queue) + sizeof(void *) * capacity);
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received NULL from malloc call in queue_create.");
        return NULL;
    }

    *queue = (Queue) {
            .capacity = capacity,
            .size = 0,
            .head = 0,
            .tail = 0,
            .mutex = PTHREAD_MUTEX_INITIALIZER,
            .can_insert = PTHREAD_COND_INITIALIZER,
            .can_extract = PTHREAD_COND_INITIALIZER
    };

    return queue;
}

void queue_destroy(Queue *const queue) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_destroy call with queue = NULL.");
        return;
    }

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->can_extract);
    pthread_cond_destroy(&queue->can_insert);
    free(queue);
}

bool queue_is_empty(const Queue *const queue) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_is_empty call with queue = NULL.");
        return false;
    }

    return queue->size == 0;
}

bool queue_is_full(const Queue *const queue) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_is_full call with queue = NULL.");
        return false;
    }

    return queue->size == queue->capacity;
}

void queue_insert(Queue *const queue, void *const object) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_insert call with queue = NULL.");
        return;
    }

    if (queue_is_full(queue)) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_insert call on a full queue.");
        return;
    }

    queue->buffer[queue->head] = object;
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size++;
}

void *queue_extract(Queue *const queue) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_extract call with queue = NULL.");
        return NULL;
    }

    if (queue_is_empty(queue)) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_extract call on an empty queue.");
        return NULL;
    }

    void *object = queue->buffer[queue->tail];
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size--;

    return object;
}

void queue_lock(Queue *const queue) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_lock call with queue = NULL.");
        return;
    }
    pthread_mutex_lock(&queue->mutex);
}

void queue_unlock(Queue *const queue) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_unlock call with queue = NULL.");
        return;
    }
    pthread_mutex_unlock(&queue->mutex);
}

void queue_wait_to_insert(Queue *const queue) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_wait_to_insert call with queue = NULL.");
        return;
    }
    pthread_cond_wait(&queue->can_insert, &queue->mutex);
}

void queue_wait_to_extract(Queue *const queue) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_wait_to_extract call with queue = NULL.");
        return;
    }
    pthread_cond_wait(&queue->can_extract, &queue->mutex);
}

void queue_wait_to_insert_with_timeout(Queue *const queue, const time_t seconds) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_wait_to_insert_with_timeout call with queue = NULL.");
        return;
    }

    struct timeval tp;
    struct timespec ts;

    gettimeofday(&tp, NULL);
    ts.tv_sec = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += seconds;

    pthread_cond_timedwait(&queue->can_insert, &queue->mutex, &ts);
}

void queue_wait_to_extract_with_timeout(Queue *const queue, const time_t seconds) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_wait_to_extract_with_timeout call with queue = NULL.");
        return;
    }

    struct timeval tp;
    struct timespec ts;

    gettimeofday(&tp, NULL);
    ts.tv_sec = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += seconds;

    pthread_cond_timedwait(&queue->can_extract, &queue->mutex, &ts);
}

void queue_notify_insert(Queue *const queue) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_notify_insert call with queue = NULL.");
        return;
    }
    pthread_cond_signal(&queue->can_insert);
}

void queue_notify_extract(Queue *const queue) {
    if (queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received queue_notify_extract call with queue = NULL.");
        return;
    }
    pthread_cond_signal(&queue->can_extract);
}

