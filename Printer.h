//
// Created by Admin on 16.06.2022.
//

#ifndef TIETO_PRINTER_H
#define TIETO_PRINTER_H

#include "Queue.h"

typedef struct Printer Printer;

Printer *printer_create(Queue *analyzer_printer_queue);
void printer_destroy(Printer *printer);

#endif //TIETO_PRINTER_H
