#pragma once
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

struct timer {
    uv_timer_t uv_timer;
    void (*cb)(void*);
    void* udata;
    int repeat_count;
};

void cancel_timer(struct timer* t);

struct timer* schedule_repeat(void (*cb)(void*), void* udata, int after_msec,
                              int repeat_count, int repeat_msec);

struct timer* schedule_once(void (*cb)(void*), void* udata, int after_msec);

void* get_timer_udata(struct timer* t);

#ifdef __cplusplus
}
#endif
