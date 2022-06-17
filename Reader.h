#ifndef TIETO_READER_H
#define TIETO_READER_H

#include <stdbool.h>
#include "Queue.h"
#include "Watchdog.h"

typedef struct Reader Reader;

Reader *reader_create(Queue *reader_analyzer_queue, Watchdog *watchdog);

void reader_await_and_destroy(Reader *reader);

void reader_request_stop_synchronized(Reader *reader);

#endif //TIETO_READER_H
