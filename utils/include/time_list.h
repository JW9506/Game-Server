#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct timer;

void cancel_timer(struct timer* t);

struct timer* schedule_repeat(void (*cb)(void*), void* udata, int after_msec,
                              int repeat_count, int repeat_msec);

struct timer* schedule_once(void (*cb)(void*), void* udata, int after_msec);

#ifdef __cplusplus
}
#endif
