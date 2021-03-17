#include "cache_alloc.h"
#include <stdlib.h>
#include <string.h>

#define _new(type, var) type* var = (type*)calloc(1, sizeof(type))

struct cache_allocer* create_cache_allocer(int capacity, int elem_size) {
    _new(struct cache_allocer, allocer);
    elem_size =
        elem_size < sizeof(struct node) ? sizeof(struct node) : elem_size;
    allocer->capacity = capacity;
    allocer->elem_size = elem_size;
    allocer->cache_mem = malloc(capacity * elem_size);
    memset(allocer->cache_mem, 0, capacity * elem_size);
    allocer->free_list = NULL;

    for (int i = 0; i < capacity; ++i) {
        struct node* walk = (struct node*)(allocer->cache_mem + i * elem_size);
        walk->next = allocer->free_list;
        allocer->free_list = walk;
    }
    return allocer;
}

void destroy_cache_allocer(struct cache_allocer* allocer) {
    if (allocer->cache_mem != NULL) { free(allocer->cache_mem); }
    free(allocer);
}

void* cache_alloc(struct cache_allocer* allocer, int elem_size) {
    if (!allocer || allocer->elem_size < elem_size) {
        return malloc(elem_size);
    }
    if (allocer->free_list != NULL) {
        void* now = allocer->free_list;
        allocer->free_list = allocer->free_list->next;
        return now;
    }
    return malloc(elem_size);
}

void cache_free(struct cache_allocer* allocer, void* mem) {
    if (allocer && (unsigned char*)mem >= allocer->cache_mem &&
        (unsigned char*)mem <
            allocer->cache_mem + allocer->capacity * allocer->elem_size) {
        struct node* node = mem;
        node->next = allocer->free_list;
        allocer->free_list = node;
        return;
    }
    free(mem);
}
