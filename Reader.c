//
// Created by Admin on 16.06.2022.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "Reader.h"

static const size_t READER_CHAR_BUFFER_SIZE = 4096;

static struct timespec READER_UPDATE_INTERVAL = { .tv_sec = 1, .tv_nsec = 0 };

struct Reader {
    Queue* reader_analyzer_queue;
    Watchdog* watchdog;
    size_t watchdog_index;
    pthread_t thread;
    pthread_mutex_t mutex;
    bool should_stop;
    char padding[7];
};

static bool reader_should_stop_synchronized(Reader* reader) {
    if(reader == NULL) {
        perror("reader_should_stop_synchronized called on NULL");
        return true;
    }

    bool return_value;
    pthread_mutex_lock(&reader->mutex);
    return_value = reader->should_stop;
    pthread_mutex_unlock(&reader->mutex);
    return return_value;
}

void reader_request_stop_synchronized(Reader* reader) {
    if(reader == NULL) {
        perror("reader_request_stop_synchronized called on NULL");
        return;
    }

    pthread_mutex_lock(&reader->mutex);
    reader->should_stop = true;
    pthread_mutex_unlock(&reader->mutex);

    queue_lock(reader->reader_analyzer_queue);
    queue_notify_insert(reader->reader_analyzer_queue);
    queue_unlock(reader->reader_analyzer_queue);
}

static void reader_request_stop_synchronized_void(void* reader) {
    reader_request_stop_synchronized((Reader*) reader);
}

static void *reader_thread(void* args) {
    Reader* reader = (Reader*) args;

    FILE *proc_file = fopen("/proc/stat", "r");
    if(proc_file == NULL) {
        perror("fopen error");
        return NULL;
    }

    if(setvbuf(proc_file, NULL, _IONBF, 0) != 0) {
        perror("setvbuf error");
        fclose(proc_file);
        return NULL;
    }

    while(!reader_should_stop_synchronized(reader)) {
        watchdog_update(reader->watchdog, reader->watchdog_index);

        char *buffer = malloc(sizeof(char) * READER_CHAR_BUFFER_SIZE);
        if(buffer == NULL) {
            perror("malloc error");
            break;
        }

        size_t read = fread(buffer, sizeof(char), READER_CHAR_BUFFER_SIZE - 1, proc_file);
        if(ferror(proc_file)) {
            perror("fread error");
            free(buffer);
            break;
        }
        buffer[read] = '\0';

        if(read >= READER_CHAR_BUFFER_SIZE) {
            perror("READER_CHAR_BUFFER_SIZE too small");
        }

        queue_lock(reader->reader_analyzer_queue);
        while(queue_is_full(reader->reader_analyzer_queue)) {
            queue_wait_to_insert(reader->reader_analyzer_queue);
            if(reader_should_stop_synchronized(reader)) {
                queue_unlock(reader->reader_analyzer_queue);
                free(buffer);
                fclose(proc_file);
                return NULL;
            }
        }
        queue_insert(reader->reader_analyzer_queue, buffer);
        queue_notify_extract(reader->reader_analyzer_queue);
        queue_unlock(reader->reader_analyzer_queue);

        rewind(proc_file);
        if(ferror(proc_file)) {
            perror("rewind error");
            break;
        }

        nanosleep(&READER_UPDATE_INTERVAL, NULL);
    }
    fclose(proc_file);
    return NULL;
}

Reader *reader_create(Queue *reader_analyzer_queue, Watchdog* watchdog) {
    if(reader_analyzer_queue == NULL) {
        perror("reader_analyzer_queue NULL");
        return NULL;
    }

    if(watchdog == NULL) {
        perror("reader_create watchdog NULL");
        return NULL;
    }

    Reader *reader = malloc(sizeof(Reader));
    if(reader == NULL) {
        perror("malloc error");
        return NULL;
    }

    *reader = (Reader) {
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .should_stop = false,
        .reader_analyzer_queue = reader_analyzer_queue,
        .watchdog = watchdog,
        .watchdog_index = watchdog_register_watch(watchdog, &reader_request_stop_synchronized_void, reader)
    };

    if(pthread_create(&reader->thread, NULL, reader_thread, (void*) reader) != 0) {
        perror("pthread_create error");
        pthread_mutex_destroy(&reader->mutex);
        free(reader);
        return NULL;
    }

    return reader;
}

void reader_await_and_destroy(Reader* reader) {
    if(reader == NULL) {
        return;
    }

    pthread_join(reader->thread, NULL);
    pthread_mutex_destroy(&reader->mutex);
    free(reader);
}


