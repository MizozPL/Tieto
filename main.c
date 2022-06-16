#include <stdio.h>
#include <unistd.h>

#include "Reader.h"
#include "Analyzer.h"
#include "Printer.h"
#include "LongDoubleArray.h"
#include "Watchdog.h"

static const size_t READER_ANALYZER_QUEUE_CAPACITY = 10;
static const size_t ANALYZER_PRINTER_QUEUE_CAPACITY = 10;

int main() {
    Queue *reader_analyzer_queue = queue_create(READER_ANALYZER_QUEUE_CAPACITY);
    Queue *analyzer_printer_queue = queue_create(ANALYZER_PRINTER_QUEUE_CAPACITY);

    Watchdog* watchdog = watchdog_create(3);
    Reader *reader = reader_create(reader_analyzer_queue, watchdog);
    Analyzer *analyzer = analyzer_create(reader_analyzer_queue, analyzer_printer_queue, watchdog);
    Printer *printer = printer_create(analyzer_printer_queue, watchdog);

    sleep(3);
    watchdog_start_watching(watchdog);

    reader_await_and_destroy(reader);
    analyzer_await_and_destroy(analyzer);
    printer_await_and_destroy(printer);
    watchdog_await_and_destroy(watchdog);

    while(!queue_is_empty(reader_analyzer_queue)) {
        void* object = queue_extract(reader_analyzer_queue);
        free(object);
    }

    while(!queue_is_empty(analyzer_printer_queue)) {
        LongDoubleArray* object = queue_extract(analyzer_printer_queue);
        long_double_array_destroy(object);
    }

    queue_destroy(reader_analyzer_queue);
    queue_destroy(analyzer_printer_queue);

    return 0;
}
