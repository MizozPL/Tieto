#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "Reader.h"
#include "Analyzer.h"
#include "Printer.h"
#include "LongDoubleArray.h"
#include "Watchdog.h"

static const size_t READER_ANALYZER_QUEUE_CAPACITY = 10;
static const size_t ANALYZER_PRINTER_QUEUE_CAPACITY = 10;

static bool running = false;
static Watchdog* watchdog;
static Reader *reader;
static Analyzer *analyzer;
static Printer *printer;

static void sighandler(int signo) {
    if(signo == SIGTERM) {
        if(running) {
            printf("Stopping!\n");
            watchdog_request_stop_synchronized(watchdog);
            reader_request_stop_synchronized(reader);
            analyzer_request_stop_synchronized(analyzer);
            printer_request_stop_synchronized(printer);
            running = false;
        }
    }
}

int main() {
    //Flags for children
    sigset_t set_blocked;
    sigemptyset(&set_blocked);
    sigaddset(&set_blocked, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set_blocked, NULL);

    Queue *reader_analyzer_queue = queue_create(READER_ANALYZER_QUEUE_CAPACITY);
    Queue *analyzer_printer_queue = queue_create(ANALYZER_PRINTER_QUEUE_CAPACITY);

    watchdog = watchdog_create(3);
    reader = reader_create(reader_analyzer_queue, watchdog);
    analyzer = analyzer_create(reader_analyzer_queue, analyzer_printer_queue, watchdog);
    printer = printer_create(analyzer_printer_queue, watchdog);
    running = true;

    //Reset flags
    pthread_sigmask(SIG_UNBLOCK, &set_blocked, NULL);

    //Install signal handler
    struct sigaction act;
    act.sa_handler = &sighandler;
    sigfillset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &act, NULL);

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
