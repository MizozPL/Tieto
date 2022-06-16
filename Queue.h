//
// Created by Admin on 16.06.2022.
//

#ifndef TIETO_QUEUE_H
#define TIETO_QUEUE_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct Queue Queue;

Queue* queue_create(size_t capacity);
void queue_destroy(Queue* queue);
bool queue_is_empty(const Queue* queue);
bool queue_is_full(const Queue* queue);
void queue_insert(Queue* queue, void* object);
void* queue_extract(Queue* queue);
void queue_lock(Queue* queue);
void queue_unlock(Queue* queue);
void queue_wait_to_insert(Queue* queue);
void queue_wait_to_extract(Queue* queue);
void queue_wait_to_insert_with_timeout(Queue* queue, time_t seconds);
void queue_wait_to_extract_with_timeout(Queue* queue, time_t seconds);
void queue_notify_insert(Queue* queue);
void queue_notify_extract(Queue* queue);

#endif //TIETO_QUEUE_H
