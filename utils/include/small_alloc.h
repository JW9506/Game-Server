#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void* small_alloc(int size);
void small_free(void* mem);

#ifdef __cplusplus
}
#endif
