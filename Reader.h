//
// Created by Admin on 16.06.2022.
//

#ifndef TIETO_READER_H
#define TIETO_READER_H

#include <stdbool.h>

typedef struct Reader Reader;

const size_t READER_CHAR_BUFFER_SIZE;
Reader *reader_create();
void reader_destroy(Reader *reader);

#endif //TIETO_READER_H
