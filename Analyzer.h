//
// Created by Admin on 16.06.2022.
//

#ifndef TIETO_ANALYZER_H
#define TIETO_ANALYZER_H

#include "Queue.h"
#include "Watchdog.h"

typedef struct Analyzer Analyzer;

void analyzer_request_stop_synchronized(Analyzer *analyzer);
Analyzer *analyzer_create(Queue *reader_analyzer_queue, Queue *analyzer_printer_queue, Watchdog* watchdog);
void analyzer_await_and_destroy(Analyzer *analyzer);

#endif //TIETO_ANALYZER_H
