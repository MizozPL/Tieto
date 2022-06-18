#include "../include/LongDoubleArray.h"

LongDoubleArray *long_double_array_create(const size_t num_elements) {
    LongDoubleArray *array = malloc(sizeof(LongDoubleArray) + sizeof(long double) * num_elements);
    *array = (LongDoubleArray) {
            .num_elements = num_elements,
    };
    return array;
}

void long_double_array_destroy(LongDoubleArray *const array) {
    free(array);
}

