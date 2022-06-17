#include "Analyzer.h"
#include "LongDoubleArray.h"
#include "Watchdog.h"
#include "Logger.h"
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

struct Analyzer {
    Queue *reader_analyzer_queue;
    Queue *analyzer_printer_queue;
    Watchdog *watchdog;
    size_t watchdog_index;
    pthread_t thread;
    pthread_mutex_t mutex;
    bool should_stop;
};

typedef struct CpuData {
    unsigned long long int total_time;
    unsigned long long int idle_time;
} CpuData;

static void analyzer_request_stop_synchronized_void(void *analyzer);

static bool analyzer_should_stop_synchronized(Analyzer *analyzer);

static size_t analyze_cpu_count(char *input);

static void analyze_line(char *line, CpuData *cpu_data);

static void *analyzer_thread(void *args);

Analyzer *analyzer_create(Queue *reader_analyzer_queue, Queue *analyzer_printer_queue, Watchdog *watchdog) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_create: Entry.");

    if (reader_analyzer_queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received analyzer_create call with reader_analyzer_queue = NULL.");
        return NULL;
    }

    if (analyzer_printer_queue == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received analyzer_create call with analyzer_printer_queue = NULL.");
        return NULL;
    }

    if (watchdog == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received printer_create call with watchdog = NULL.");
        return NULL;
    }

    Analyzer *analyzer = malloc(sizeof(Analyzer));
    if (analyzer == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received NULL from malloc call in analyzer_create.");
        return NULL;
    }

    *analyzer = (Analyzer) {
            .reader_analyzer_queue = reader_analyzer_queue,
            .analyzer_printer_queue = analyzer_printer_queue,
            .watchdog = watchdog,
            .watchdog_index = watchdog_register_watch(watchdog, &analyzer_request_stop_synchronized_void, analyzer),
            .mutex = PTHREAD_MUTEX_INITIALIZER,
            .should_stop = false
    };

    if (pthread_create(&analyzer->thread, NULL, analyzer_thread, (void *) analyzer) != 0) {
        logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received error from pthread_create in analyzer_create.");
        pthread_mutex_destroy(&analyzer->mutex);
        free(analyzer);
        return NULL;
    }

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_create: Success.");
    return analyzer;
}

void analyzer_await_and_destroy(Analyzer *analyzer) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_await_and_destroy: Entry.");

    if (analyzer == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received analyzer_await_and_destroy call with analyzer = NULL.");
        return;
    }

    pthread_join(analyzer->thread, NULL);
    pthread_mutex_destroy(&analyzer->mutex);
    free(analyzer);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_await_and_destroy: Success.");
}

void analyzer_request_stop_synchronized(Analyzer *analyzer) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_request_stop_synchronized: Entry.");

    if (analyzer == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received analyzer_request_stop_synchronized call with analyzer = NULL.");
        return;
    }

    pthread_mutex_lock(&analyzer->mutex);
    analyzer->should_stop = true;
    pthread_mutex_unlock(&analyzer->mutex);

    queue_lock(analyzer->reader_analyzer_queue);
    queue_notify_extract(analyzer->reader_analyzer_queue);
    queue_unlock(analyzer->reader_analyzer_queue);

    queue_lock(analyzer->analyzer_printer_queue);
    queue_notify_insert(analyzer->analyzer_printer_queue);
    queue_unlock(analyzer->analyzer_printer_queue);

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_request_stop_synchronized: Success.");
}

static void analyzer_request_stop_synchronized_void(void *analyzer) {
    analyzer_request_stop_synchronized((Analyzer *) analyzer);
}

static bool analyzer_should_stop_synchronized(Analyzer *analyzer) {
    if (analyzer == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                   "Received analyzer_should_stop_synchronized call with analyzer = NULL.");
        return false;
    }

    bool return_value;
    pthread_mutex_lock(&analyzer->mutex);
    return_value = analyzer->should_stop;
    pthread_mutex_unlock(&analyzer->mutex);

    return return_value;
}

static size_t analyze_cpu_count(char *input) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyze_cpu_count: Entry.");

    if (input == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN, "Received analyze_cpu_count call with input = NULL.");
        return 0;
    }

    size_t cpu_count = 0;

    size_t parse_length = strlen(input);
    for (size_t i = 2; i < parse_length; i++) {
        if (input[i - 2] == 'c' && input[i - 1] == 'p' && input[i] == 'u') {
            cpu_count++;
        }
    }

    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyze_cpu_count: Success.");
    return cpu_count;
}

static void analyze_line(char *line, CpuData *cpu_data) {
    if (cpu_data == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN, "Received cpu_data call with cpu_data = NULL.");
        return;
    }

    if (line == NULL) {
        logger_log(logger_get_global(), LOGGER_LEVEL_WARN, "Received cpu_data call with line = NULL.");
        *cpu_data = (CpuData) {
                .idle_time = 0,
                .total_time = 0,
        };
        return;
    }

    unsigned long long int user, nice, system, idle, io_wait, irq, soft_irq, steal, guest, guest_nice;
    sscanf(line, "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &io_wait, &irq,
           &soft_irq, &steal, &guest, &guest_nice);

    user -= guest;
    nice -= guest_nice;

    *cpu_data = (CpuData) {
            .idle_time = idle + io_wait,
            .total_time = user + nice + system + irq + soft_irq + idle + io_wait + steal + guest + guest_nice
    };
}

static void *analyzer_thread(void *args) {
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_thread: Entry.");

    Analyzer *analyzer = (Analyzer *) args;
    size_t cpu_count = 0;

    CpuData *previous_cpu_data = NULL;
    while (!analyzer_should_stop_synchronized(analyzer)) {
        logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_thread: Iteration.");
        watchdog_update(analyzer->watchdog, analyzer->watchdog_index);

        queue_lock(analyzer->reader_analyzer_queue);
        while (queue_is_empty(analyzer->reader_analyzer_queue)) {
            queue_wait_to_extract(analyzer->reader_analyzer_queue);
            if (analyzer_should_stop_synchronized(analyzer)) {
                queue_unlock(analyzer->reader_analyzer_queue);
                free(previous_cpu_data);
                logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_thread: Ending.");
                return NULL;
            }
        }

        char *input = queue_extract(analyzer->reader_analyzer_queue);
        queue_notify_insert(analyzer->reader_analyzer_queue);
        queue_unlock(analyzer->reader_analyzer_queue);

        if (cpu_count == 0) {
            cpu_count = analyze_cpu_count(input);
        }

        CpuData *cpu_data = malloc(sizeof(CpuData) * cpu_count);
        if (cpu_data == NULL) {
            logger_log(logger_get_global(), LOGGER_LEVEL_ERROR, "Received NULL from malloc call in analyzer_thread.");
            return NULL;
        }

        char *token = strtok(input, "\n");
        for (size_t i = 0; i < cpu_count; i++) {
            analyze_line(token, &cpu_data[i]);
            token = strtok(NULL, "\n");
        }
        free(input);

        if (previous_cpu_data == NULL) {
            previous_cpu_data = cpu_data;
            continue;
        }

        bool error = false;
        LongDoubleArray *array = long_double_array_create(cpu_count);
        for (size_t i = 0; i < cpu_count; i++) {
            unsigned long long int total_time_diff = cpu_data[i].total_time - previous_cpu_data[i].total_time;
            unsigned long long int idle_time_diff = cpu_data[i].idle_time - previous_cpu_data[i].idle_time;
            if (total_time_diff == 0) {
                logger_log(logger_get_global(), LOGGER_LEVEL_WARN,
                           "Calculated total_time_diff = 0. Try increasing READER_UPDATE_INTERVAL.");
                error = true;
            }
            long double percentage = (total_time_diff - idle_time_diff) / ((long double) total_time_diff);
            array->buffer[i] = percentage;
        }

        if (!error) {
            queue_lock(analyzer->analyzer_printer_queue);
            while (queue_is_full(analyzer->analyzer_printer_queue)) {
                queue_wait_to_insert(analyzer->analyzer_printer_queue);
                if (analyzer_should_stop_synchronized(analyzer)) {
                    queue_unlock(analyzer->analyzer_printer_queue);
                    free(cpu_data);
                    free(previous_cpu_data);
                    long_double_array_destroy(array);
                    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_thread: Ending.");
                    return NULL;
                }
            }
            queue_insert(analyzer->analyzer_printer_queue, array);
            queue_notify_extract(analyzer->analyzer_printer_queue);
            queue_unlock(analyzer->analyzer_printer_queue);
        }

        free(previous_cpu_data);
        previous_cpu_data = cpu_data;
    }

    free(previous_cpu_data);
    logger_log(logger_get_global(), LOGGER_LEVEL_DEBUG, "analyzer_thread: Ending.");
    return NULL;
}
