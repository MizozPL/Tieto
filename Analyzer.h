//
// Created by Admin on 16.06.2022.
//

#ifndef TIETO_ANALYZER_H
#define TIETO_ANALYZER_H

#include "Queue.h"

typedef struct Analyzer Analyzer;
Analyzer *analyzer_create(Queue *reader_analyzer_queue, Queue *analyzer_printer_queue);
void analyzer_destroy(Analyzer *analyzer);

#endif //TIETO_ANALYZER_H
