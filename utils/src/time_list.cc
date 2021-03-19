#include "time_list.h"
#include <stdlib.h>
#include <uv.h>

typedef void (*_timer_cb)(void*);

#define _new(type, var) type* var = (type*)calloc(1, sizeof(type))
#define _free           free

static struct timer* alloc_timer(_timer_cb cb, void* udata, int repeat_count) {
    _new(struct timer, t);
    t->cb = cb;
    t->repeat_count = repeat_count;
    t->udata = udata;
    uv_timer_init(uv_default_loop(), &t->uv_timer);
    return t;
}

static void free_timer(struct timer* t) { _free(t); }

static void on_uv_timer(uv_timer_t* handle) {
    struct timer* t = (struct timer*)handle->data;
    if (t->repeat_count < 0) {
        t->cb(t->udata);
    } else {
        --t->repeat_count;
        t->cb(t->udata);
        if (t->repeat_count == 0) {
            uv_timer_stop(&t->uv_timer);
            free_timer(t);
        }
    }
}

void cancel_timer(struct timer* t) {
    if (t->repeat_count == 0) { return; }
    uv_timer_stop(&t->uv_timer);
    free_timer(t);
}

struct timer* schedule_repeat(_timer_cb cb, void* udata, int after_msec,
                              int repeat_count, int repeat_msec) {
    struct timer* t = alloc_timer(cb, udata, repeat_count);
    t->uv_timer.data = t;
    uv_timer_start(&t->uv_timer, on_uv_timer, after_msec, repeat_msec);
    return t;
}

struct timer* schedule_once(_timer_cb cb, void* udata, int after_msec) {
    return schedule_repeat(cb, udata, after_msec, 1, after_msec);
}

void* get_timer_udata(struct timer* t) { return t->udata; }
