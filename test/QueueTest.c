#include <assert.h>
#include "../include/Queue.h"
#include "../include/Logger.h"

int main(void)
{
    int array[] = {0, 1, 2, 3, 4};
    size_t array_size = sizeof(array)/sizeof(int);

    int full_size_val = 111;

    Queue *queue = queue_create(5);
    assert(queue_is_empty(queue));
    assert(!queue_is_full(queue));
    assert(queue_extract(queue) == NULL);

    queue_insert(queue,&array[0]);
    assert(!queue_is_empty(queue));
    assert(!queue_is_full(queue));

    queue_insert(queue,&array[1]);
    assert(!queue_is_empty(queue));
    assert(!queue_is_full(queue));

    queue_insert(queue,&array[2]);
    assert(!queue_is_empty(queue));
    assert(!queue_is_full(queue));

    queue_insert(queue,&array[3]);
    assert(!queue_is_empty(queue));
    assert(!queue_is_full(queue));

    queue_insert(queue,&array[4]);
    assert(!queue_is_empty(queue));
    assert(queue_is_full(queue));

    queue_insert(queue, &full_size_val);

    for(size_t i = 0; i < array_size; i++) {
        assert(queue_extract(queue) == &array[i]);
    }

    assert(queue_extract(queue) == NULL);
    assert(queue_is_empty(queue));
    assert(!queue_is_full(queue));

    queue_destroy(queue);
    logger_destroy(logger_get_global());
    return 0;
}
