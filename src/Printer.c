#include <stdio.h>
#include <pthread.h>
#include "../include/Printer.h"
#include "../include/LongDoubleArray.h"
#include "../include/Logger.h"

struct Printer {
    Queue *analyzer_printer_queue;
    Watchdog *watchdog;
    size_t watchdog_index;
    pthread_t thread;
    pthread_mutex_t mutex;
    bool should_stop;
};

static void printer_request_stop_synchronized_void(void *printer);

static bool printer_should_stop_synchronized(Printer *printer);

static void *printer_thread(void *args);

Printer *printer_create(Queue *const analyzer_printer_queue, Watchdog *const watchdog) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_create: Entry.");

    if (analyzer_printer_queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received printer_create call with analyzer_printer_queue = NULL.");
        return NULL;
    }

    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received printer_create call with watchdog = NULL.");
        return NULL;
    }

    Printer *printer = malloc(sizeof(Printer));
    if (printer == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received NULL from malloc call in printer_create.");
        return NULL;
    }

    *printer = (Printer) {
            .analyzer_printer_queue = analyzer_printer_queue,
            .watchdog = watchdog,
            .watchdog_index = watchdog_register_watch(watchdog, &printer_request_stop_synchronized_void, printer),
            .mutex = PTHREAD_MUTEX_INITIALIZER,
            .should_stop = false
    };

    if (pthread_create(&printer->thread, NULL, printer_thread, (void *) printer) != 0) {
        logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received error from pthread_create in printer_create.");
        pthread_mutex_destroy(&printer->mutex);
        free(printer);
        return NULL;
    }

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_create: Success.");
    return printer;
}

void printer_await_and_destroy(Printer *const printer) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_await_and_destroy: Entry.");

    if (printer == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received printer_await_and_destroy call with printer = NULL.");
        return;
    }

    pthread_join(printer->thread, NULL);
    pthread_mutex_destroy(&printer->mutex);
    free(printer);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_await_and_destroy: Success.");
}

void printer_request_stop_synchronized(Printer *const printer) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_request_stop_synchronized: Entry.");

    if (printer == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received printer_request_stop_synchronized call with printer = NULL.");
        return;
    }

    pthread_mutex_lock(&printer->mutex);
    printer->should_stop = true;
    pthread_mutex_unlock(&printer->mutex);

    queue_lock(printer->analyzer_printer_queue);
    queue_notify_extract(printer->analyzer_printer_queue);
    queue_unlock(printer->analyzer_printer_queue);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_request_stop_synchronized: Success.");
}

static void printer_request_stop_synchronized_void(void *const printer) {
    printer_request_stop_synchronized((Printer *) printer);
}

static bool printer_should_stop_synchronized(Printer *const printer) {
    if (printer == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received printer_should_stop_synchronized call with printer = NULL.");
        return true;
    }

    bool return_value;
    pthread_mutex_lock(&printer->mutex);
    return_value = printer->should_stop;
    pthread_mutex_unlock(&printer->mutex);

    return return_value;
}

static void *printer_thread(void *args) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_thread: Entry.");

    Printer *printer = (Printer *) args;

    while (!printer_should_stop_synchronized(printer)) {
        logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_thread: Iteration.");
        watchdog_update(printer->watchdog, printer->watchdog_index);

        queue_lock(printer->analyzer_printer_queue);
        while (queue_is_empty(printer->analyzer_printer_queue)) {
            queue_wait_to_extract(printer->analyzer_printer_queue);
            if (printer_should_stop_synchronized(printer)) {
                queue_unlock(printer->analyzer_printer_queue);
                logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_thread: Ending.");
                return NULL;
            }
        }
        LongDoubleArray *array = queue_extract(printer->analyzer_printer_queue);
        queue_notify_insert(printer->analyzer_printer_queue);
        queue_unlock(printer->analyzer_printer_queue);

        printf("\x1b[2J\x1b[H");
        if (array->num_elements > 0) {
            printf("CPU:\t%.2Lf\n", array->buffer[0]);
        }
        for (size_t i = 1; i < array->num_elements; i++) {
            printf("CPU%zu:\t%.2Lf\n", i - 1, array->buffer[i]);
        }
        printf("\n");

        long_double_array_destroy(array);
    }

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "printer_thread: Ending.");
    return NULL;
}
