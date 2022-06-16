//
// Created by Admin on 17.06.2022.
//

#ifndef TIETO_LOGGER_H
#define TIETO_LOGGER_H

typedef struct Logger Logger;

enum LOGGER_LEVEL {
    LOGGER_LEVEL_DEBUG, LOGGER_LEVEL_INFO, LOGGER_LEVEL_WARN, LOGGER_LEVEL_ERROR
};

Logger* logger_create(char* path);
Logger* logger_get_global(void);
void logger_log(Logger* logger, enum LOGGER_LEVEL level, char* message);
void logger_destroy(Logger* logger);

#endif //TIETO_LOGGER_H
