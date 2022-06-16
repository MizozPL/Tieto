//
// Created by Admin on 16.06.2022.
//

#ifndef TIETO_PRINTER_H
#define TIETO_PRINTER_H

#include "Queue.h"
#include "Watchdog.h"

typedef struct Printer Printer;

void printer_request_stop_synchronized(Printer* printer);
Printer *printer_create(Queue *analyzer_printer_queue, Watchdog* watchdog);
void printer_await_and_destroy(Printer *printer);

#endif //TIETO_PRINTER_H
