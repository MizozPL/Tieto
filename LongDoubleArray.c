//
// Created by Admin on 16.06.2022.
//

#include "LongDoubleArray.h"

LongDoubleArray* long_double_array_create(size_t num_elements) {
    LongDoubleArray* array = malloc(sizeof(LongDoubleArray) + sizeof(long double) * num_elements);
    *array = (LongDoubleArray) {
        .num_elements = num_elements,
    };
    return array;
}

void long_double_array_destroy(LongDoubleArray* array) {
    free(array);
}

