//
// Created by Admin on 16.06.2022.
//

#ifndef TIETO_READER_H
#define TIETO_READER_H

#include <stdbool.h>
#include "Queue.h"
#include "Watchdog.h"

typedef struct Reader Reader;

void reader_request_stop_synchronized(Reader* reader);
Reader *reader_create(Queue *reader_analyzer_queue, Watchdog* watchdog);
void reader_await_and_destroy(Reader *reader);

#endif //TIETO_READER_H
