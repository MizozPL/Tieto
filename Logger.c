//
// Created by Admin on 17.06.2022.
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "Logger.h"

static pthread_mutex_t global_logger_mutex = PTHREAD_MUTEX_INITIALIZER;
static Logger* global_logger = NULL;
static const char *enum_names[] = {"DEBUG", "INFO", "WARN", "ERROR"};

struct Logger {
    FILE *log_file;
    pthread_mutex_t mutex;
};

Logger* logger_create(char* path) {
    FILE *log_file = fopen(path, "w");
    if(log_file == NULL) {
        perror("fopen error");
        return NULL;
    }

    Logger* logger = malloc(sizeof(Logger));
    if(logger == NULL) {
        perror("malloc error");
        return NULL;
    }

    *logger = (Logger) {
      .log_file = log_file,
      .mutex = PTHREAD_MUTEX_INITIALIZER
    };

    return logger;
}

Logger* logger_get_global(void) {
    if(global_logger == NULL) {
        pthread_mutex_lock(&global_logger_mutex);
        if(global_logger == NULL) {
            global_logger = logger_create("./global.log");
        }
        pthread_mutex_unlock(&global_logger_mutex);
    }
    return global_logger;
}

void logger_log(Logger* logger, enum LOGGER_LEVEL level, char* message) {
    if(logger == NULL) {
        perror("logger_log on NULL");
        return;
    }
    if(message == NULL) {
        perror("logger_log message NULL");
        return;
    }

    char buffer[50];
    time_t current_time = time(NULL);
    char* time_string = ctime_r(&current_time, buffer);

    if(time_string == NULL) {
        perror("time_string NULL");
        return;
    }

    char* end_line = memchr(time_string, '\n', strlen(time_string));
    *end_line = '\0';

    pthread_mutex_lock(&logger->mutex);
    fprintf(logger->log_file, "[%s] [%s] %s\n", enum_names[level], time_string, message);
    fflush(logger->log_file);
    pthread_mutex_unlock(&logger->mutex);
}

void logger_destroy(Logger* logger) {
    if(logger == NULL) {
        perror("logger_destroy on NULL");
        return;
    }

    fclose(logger->log_file);
    free(logger);
}

