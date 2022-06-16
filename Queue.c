//
// Created by Admin on 16.06.2022.
//

#include "Queue.h"
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
    void* buffer[];
};

Queue* queue_create(size_t capacity) {
    if(capacity <= 0) {
        perror("queue_create capacity <= 0");
        return NULL;
    }

    Queue* queue = malloc(sizeof(Queue) + sizeof(void*) * capacity);
    if(queue == NULL) {
        perror("malloc error");
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

void queue_destroy(Queue* queue) {
    if(queue == NULL) {
        perror("queue_destroy on NULL");
        return;
    }

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->can_extract);
    pthread_cond_destroy(&queue->can_insert);
    free(queue);
}

bool queue_is_empty(const Queue* queue) {
    if(queue == NULL) {
        perror("queue_is_empty on NULL");
        return false;
    }

    return queue->size == 0;
}

bool queue_is_full(const Queue* queue) {
    if(queue == NULL) {
        perror("queue_is_full on NULL");
        return false;
    }

    return queue->size == queue->capacity;
}

void queue_insert(Queue* queue, void* object) {
    if(queue == NULL) {
        perror("queue_insert on NULL");
        return;
    }

    if(queue_is_full(queue)) {
        perror("queue_insert on a full queue");
        return;
    }

    queue->buffer[queue->head] = object;
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size++;
}

void* queue_extract(Queue* queue) {
    if(queue == NULL) {
        perror("queue_extract on NULL");
        return NULL;
    }

    if(queue_is_empty(queue)) {
        perror("queue_extract on an empty queue");
        return NULL;
    }

    void* object = queue->buffer[queue->tail];
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size--;

    return object;
}

void queue_lock(Queue* queue) {
    if(queue == NULL) {
        perror("queue_lock on NULL");
        return;
    }
    pthread_mutex_lock(&queue->mutex);
}

void queue_unlock(Queue* queue) {
    if(queue == NULL) {
        perror("queue_unlock on NULL");
        return;
    }
    pthread_mutex_unlock(&queue->mutex);
}

void queue_wait_to_insert(Queue* queue) {
    pthread_cond_wait(&queue->can_insert, &queue->mutex);
}

void queue_wait_to_extract(Queue* queue) {
    pthread_cond_wait(&queue->can_extract, &queue->mutex);
}

void queue_wait_to_insert_with_timeout(Queue* queue, time_t seconds) {
    struct timeval tp;
    struct timespec ts;

    gettimeofday(&tp, NULL);
    ts.tv_sec = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += seconds;

    pthread_cond_timedwait(&queue->can_insert, &queue->mutex, &ts);
}

void queue_wait_to_extract_with_timeout(Queue* queue, time_t seconds) {
    struct timeval tp;
    struct timespec ts;

    gettimeofday(&tp, NULL);
    ts.tv_sec = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += seconds;

    pthread_cond_timedwait(&queue->can_extract, &queue->mutex, &ts);
}

void queue_notify_insert(Queue* queue) {
    pthread_cond_signal(&queue->can_insert);
}

void queue_notify_extract(Queue* queue) {
    pthread_cond_signal(&queue->can_extract);
}

