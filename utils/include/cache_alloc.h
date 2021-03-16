#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct node {
    struct node* next;
};

struct cache_allocer {
    unsigned char* cache_mem;
    int capacity;
    struct node* free_list;
    int elem_size;
};

struct cache_allocer* create_cache_allocer(int capacity, int elem_size);

void destroy_cache_allocer(struct cache_allocer* allocer);

void* cache_alloc(struct cache_allocer* allocer, int elem_size);

void cache_free(struct cache_allocer* allocer, void* mem);

#ifdef __cplusplus
}
#endif
