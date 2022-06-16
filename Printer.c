//
// Created by Admin on 16.06.2022.
//

#include <stdio.h>
#include <pthread.h>
#include "Printer.h"
#include "LongDoubleArray.h"

struct Printer {
    Queue* analyzer_printer_queue;
    pthread_t thread;
    pthread_mutex_t mutex;
    bool should_stop;
    char padding[7];
};

static bool printer_should_stop_synchronized(Printer* printer) {
    if(printer == NULL) {
        perror("printer_should_stop_synchronized called on NULL");
        return true;
    }

    bool return_value;
    pthread_mutex_lock(&printer->mutex);
    return_value = printer->should_stop;
    pthread_mutex_unlock(&printer->mutex);
    return return_value;
}

static void printer_request_stop_synchronized(Printer* printer) {
    if(printer == NULL) {
        perror("printer_request_stop_synchronized called on NULL");
        return;
    }

    pthread_mutex_lock(&printer->mutex);
    printer->should_stop = true;
    pthread_mutex_unlock(&printer->mutex);
}

static void *printer_thread(void* args) {
    Printer* printer = (Printer*) args;

    while(!printer_should_stop_synchronized(printer)) {
        queue_lock(printer->analyzer_printer_queue);
        while(queue_is_empty(printer->analyzer_printer_queue)) {
            queue_wait_to_extract(printer->analyzer_printer_queue);
            if(printer_should_stop_synchronized(printer)) {
                queue_unlock(printer->analyzer_printer_queue);
                return NULL;
            }
        }
        LongDoubleArray* array = queue_extract(printer->analyzer_printer_queue);
        queue_notify_insert(printer->analyzer_printer_queue);
        queue_unlock(printer->analyzer_printer_queue);

        printf("\x1b[2J\x1b[H");
        if(array->num_elements > 0) {
            printf("CPU:\t%.1Lf\n", array->buffer[0]);
        }
        for(size_t i = 1; i < array->num_elements; i++) {
            printf("CPU%zu:\t%.1Lf\n", i - 1, array->buffer[i]);
        }
        printf("\n");

        long_double_array_destroy(array);
    }
    return NULL;
}

Printer *printer_create(Queue *analyzer_printer_queue) {
    if(analyzer_printer_queue == NULL) {
        perror("analyzer_printer_queue on NULL");
        return NULL;
    }

    Printer *printer = malloc(sizeof(Printer));
    if(printer == NULL) {
        perror("malloc error");
        return NULL;
    }

    *printer = (Printer) {
            .mutex = PTHREAD_MUTEX_INITIALIZER,
            .should_stop = false,
            .analyzer_printer_queue = analyzer_printer_queue
    };

    if(pthread_create(&printer->thread, NULL, printer_thread, (void*) printer) != 0) {
        perror("pthread_create error");
        pthread_mutex_destroy(&printer->mutex);
        free(printer);
        return NULL;
    }

    return printer;
}

void printer_destroy(Printer* printer) {
    if(printer == NULL) {
        return;
    }

    printer_request_stop_synchronized(printer);

    queue_lock(printer->analyzer_printer_queue);
    queue_notify_extract(printer->analyzer_printer_queue);
    queue_unlock(printer->analyzer_printer_queue);

    pthread_join(printer->thread, NULL);
    pthread_mutex_destroy(&printer->mutex);
    free(printer);
}

