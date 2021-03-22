#include "small_alloc.h"
#include "cache_alloc.h"
#include <stdlib.h>

#define SMALL_ELEM_NUM  10 * 1024
#define SMALL_ELEM_SIZE 128

static struct cache_allocer* small_allocer = NULL;

void* small_alloc(int size) {
    if (!small_allocer) {
        small_allocer = create_cache_allocer(SMALL_ELEM_NUM, SMALL_ELEM_SIZE);
    }
    return cache_alloc(small_allocer, size);
}

void small_free(void* mem) {
    if (!small_allocer) {
        small_allocer = create_cache_allocer(SMALL_ELEM_NUM, SMALL_ELEM_SIZE);
    }
    cache_free(small_allocer, mem);
}
