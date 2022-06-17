#ifndef TIETO_LONGDOUBLEARRAY_H
#define TIETO_LONGDOUBLEARRAY_H

#include <stdlib.h>

typedef struct LongDoubleArray {
    size_t num_elements;
    long double buffer[];
} LongDoubleArray;

LongDoubleArray *long_double_array_create(size_t num_elements);

void long_double_array_destroy(LongDoubleArray *array);

#endif //TIETO_LONGDOUBLEARRAY_H
