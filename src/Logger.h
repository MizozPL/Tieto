#ifndef TIETO_LOGGER_H
#define TIETO_LOGGER_H

typedef struct Logger Logger;

enum LOGGER_LEVEL {
    LOGGER_LEVEL_DEBUG = 0, LOGGER_LEVEL_INFO = 1, LOGGER_LEVEL_WARN = 2, LOGGER_LEVEL_ERROR = 3
};

Logger *logger_create(char *path);

void logger_destroy(Logger *logger);

Logger *logger_get_global(void);

void logger_log(Logger *logger, enum LOGGER_LEVEL level, char *message);

#endif //TIETO_LOGGER_H
