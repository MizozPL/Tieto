#include <stdio.h>
#include <unistd.h>

#include "Reader.h"

int main() {
    printf("Start! %zu\n", READER_CHAR_BUFFER_SIZE);

    Reader *reader = reader_create();
    sleep(5);
    reader_destroy(reader);

    return 0;
}
