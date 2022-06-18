#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../include/Reader.h"
#include "../include/Logger.h"

static const size_t READER_CHAR_BUFFER_SIZE = 4096;

static struct timespec READER_UPDATE_INTERVAL = {.tv_sec = 1, .tv_nsec = 0};

struct Reader {
    Queue *reader_analyzer_queue;
    Watchdog *watchdog;
    size_t watchdog_index;
    pthread_t thread;
    pthread_mutex_t mutex;
    bool should_stop;
};

static void reader_request_stop_synchronized_void(void *reader);

static bool reader_should_stop_synchronized(Reader *reader);

static void *reader_thread(void *args);

Reader *reader_create(Queue *const reader_analyzer_queue, Watchdog *const watchdog) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "reader_create: Entry.");

    if (reader_analyzer_queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received reader_create call with reader_analyzer_queue = NULL.");
        return NULL;
    }

    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received printer_create call with watchdog = NULL.");
        return NULL;
    }

    Reader *reader = malloc(sizeof(Reader));
    if (reader == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received NULL from malloc call in reader_create.");
        return NULL;
    }

    *reader = (Reader) {
            .reader_analyzer_queue = reader_analyzer_queue,
            .watchdog = watchdog,
            .watchdog_index = watchdog_register_watch(watchdog, &reader_request_stop_synchronized_void, reader),
            .mutex = PTHREAD_MUTEX_INITIALIZER,
            .should_stop = false,
    };

    if (pthread_create(&reader->thread, NULL, reader_thread, (void *) reader) != 0) {
        logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received error from pthread_create in reader_create.");
        pthread_mutex_destroy(&reader->mutex);
        free(reader);
        return NULL;
    }

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "reader_create: Success.");
    return reader;
}

void reader_await_and_destroy(Reader *const reader) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "reader_await_and_destroy: Entry.");

    if (reader == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received reader_await_and_destroy call with reader = NULL.");
        return;
    }

    pthread_join(reader->thread, NULL);
    pthread_mutex_destroy(&reader->mutex);
    free(reader);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "reader_await_and_destroy: Success.");
}

void reader_request_stop_synchronized(Reader *const reader) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "reader_request_stop_synchronized: Entry.");

    if (reader == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received reader_request_stop_synchronized call with reader = NULL.");
        return;
    }

    pthread_mutex_lock(&reader->mutex);
    reader->should_stop = true;
    pthread_mutex_unlock(&reader->mutex);

    queue_lock(reader->reader_analyzer_queue);
    queue_notify_insert(reader->reader_analyzer_queue);
    queue_unlock(reader->reader_analyzer_queue);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "reader_request_stop_synchronized: Success.");
}

static void reader_request_stop_synchronized_void(void *const reader) {
    reader_request_stop_synchronized((Reader *) reader);
}

static bool reader_should_stop_synchronized(Reader *const reader) {
    if (reader == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received reader_should_stop_synchronized call with reader = NULL.");
        return true;
    }

    bool return_value;
    pthread_mutex_lock(&reader->mutex);
    return_value = reader->should_stop;
    pthread_mutex_unlock(&reader->mutex);
    return return_value;
}

static void *reader_thread(void *args) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "reader_thread: Entry.");

    Reader *reader = (Reader *) args;

    FILE *proc_file = fopen("/proc/stat", "r");
    if (proc_file == NULL) {
        perror("fopen error");
        return NULL;
    }

    if (setvbuf(proc_file, NULL, _IONBF, 0) != 0) {
        perror("setvbuf error");
        fclose(proc_file);
        return NULL;
    }

    while (!reader_should_stop_synchronized(reader)) {
        logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_thread: Iteration.");
        watchdog_update(reader->watchdog, reader->watchdog_index);

        char *buffer = malloc(sizeof(char) * READER_CHAR_BUFFER_SIZE);
        if (buffer == NULL) {
            logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received NULL from malloc call in reader_thread.");
            break;
        }

        size_t read = fread(buffer, sizeof(char), READER_CHAR_BUFFER_SIZE - 1, proc_file);
        if (ferror(proc_file) != 0) {
            free(buffer);
            logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "fread error in reader_thread.");
            break;
        }
        buffer[read] = '\0';

        if (read >= READER_CHAR_BUFFER_SIZE) {
            logger_log(logger_get_global(), LOGGER_LEVEL_WARN, "READER_CHAR_BUFFER_SIZE too small in reader_thread.");
        }

        queue_lock(reader->reader_analyzer_queue);
        while (queue_is_full(reader->reader_analyzer_queue)) {
            queue_wait_to_insert(reader->reader_analyzer_queue);
            if (reader_should_stop_synchronized(reader)) {
                queue_unlock(reader->reader_analyzer_queue);
                free(buffer);
                fclose(proc_file);
                logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "reader_thread: Ending.");
                return NULL;
            }
        }

        queue_insert(reader->reader_analyzer_queue, buffer);
        queue_notify_extract(reader->reader_analyzer_queue);
        queue_unlock(reader->reader_analyzer_queue);

        rewind(proc_file);
        if (ferror(proc_file) != 0) {
            logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "rewind error in reader_thread.");
            break;
        }

        nanosleep(&READER_UPDATE_INTERVAL, NULL);
    }
    fclose(proc_file);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "reader_thread: Ending.");
    return NULL;
}
