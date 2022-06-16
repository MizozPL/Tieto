//
// Created by Admin on 16.06.2022.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "Reader.h"

const size_t READER_CHAR_BUFFER_SIZE = 4096;

struct Reader {
    pthread_t reader_thread;
    pthread_mutex_t reader_stop_mutex;
    bool should_stop;
};

static bool reader_should_stop_synchronized(Reader* reader) {
    if(reader == NULL) {
        perror("reader_should_stop_synchronized called on NULL");
        return true;
    }

    bool return_value;
    pthread_mutex_lock(&reader->reader_stop_mutex);
    return_value = reader->should_stop;
    pthread_mutex_unlock(&reader->reader_stop_mutex);
    return return_value;
}

static void reader_request_stop_synchronized(Reader* reader) {
    if(reader == NULL) {
        perror("reader_request_stop_synchronized called on NULL");
        return;
    }

    pthread_mutex_lock(&reader->reader_stop_mutex);
    reader->should_stop = true;
    pthread_mutex_unlock(&reader->reader_stop_mutex);
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

        //Queue
        printf("%s\n", buffer);
        printf("%lu\n", strlen(buffer));
        free(buffer);

        rewind(proc_file);
        if(ferror(proc_file)) {
            perror("rewind error");
            break;
        }
    }
    fclose(proc_file);
    return NULL;
}

Reader *reader_create() {
    Reader *reader = malloc(sizeof(Reader));
    if(reader == NULL) {
        perror("malloc error");
        return NULL;
    }

    reader->should_stop = false;
    if(pthread_mutex_init(&reader->reader_stop_mutex, NULL) != 0) {
        perror("pthread_mutex_init error");
        free(reader);
        return NULL;
    }

    if(pthread_create(&(reader->reader_thread), NULL, reader_thread, (void*) reader) != 0) {
        perror("pthread_create error");
        pthread_mutex_destroy(&reader->reader_stop_mutex);
        free(reader);
        return NULL;
    }

    return reader;
}

void reader_destroy(Reader* reader) {
    if(reader == NULL) {
        return;
    }

    reader_request_stop_synchronized(reader);
    pthread_join(reader->reader_thread, NULL);
    pthread_mutex_destroy(&reader->reader_stop_mutex);
    free(reader);
}