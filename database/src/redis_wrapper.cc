#include "redis_wrapper.h"
#include <uv.h>
#include <hiredis/hiredis.h>

#define _new(type, var) type* var = (type*)calloc(1, sizeof(type))

struct connect_req {
    char* ip;
    int port;
    void (*open_cb)(const char* err, void* context, void* udata);
    char* err;
    void* context;
    void* udata;
};

struct query_req {
    void* context;
    char* cmd;
    void (*query_cb)(const char* err, redisReply* result, void* udata);
    redisReply* result;
    char* err;
    void* udata;
};

struct redis_context {
    void* conn;
    uv_mutex_t lock;
    int is_closed;
};

static void connect_work(uv_work_t* req) {
    struct connect_req* r = (struct connect_req*)req->data;
    struct timeval timeout = { 5, 0 };
    redisContext* redis_conn = redisConnectWithTimeout(r->ip, r->port, timeout);
    if (redis_conn->err) {
        r->err = _strdup(redis_conn->errstr);
        r->context = NULL;
        redisFree(redis_conn);
    } else {
        _new(struct redis_context, c);
        c->conn = (void*)redis_conn;
        r->context = (void*)c;
        uv_mutex_init(&c->lock);
    }
}

static void on_connect_work_complete(uv_work_t* req, int status) {
    struct connect_req* r = (struct connect_req*)req->data;
    r->open_cb(r->err, r->context, r->udata);
    if (r->ip) { free(r->ip); }
    if (r->err) { free(r->err); }
    free(r);
    free(req);
}

static void close_work(uv_work_t* req) {
    struct redis_context* c = (struct redis_context*)req->data;
    uv_mutex_lock(&c->lock);
    c->is_closed = 1;
    redisFree((redisContext*)c->conn);
    c->conn = NULL;
    uv_mutex_unlock(&c->lock);
    uv_mutex_destroy(&c->lock);
    free(c);
}

static void on_close_complete(uv_work_t* req, int status) { free(req); }

static void query_work(uv_work_t* req) {
    struct query_req* r = (struct query_req*)req->data;
    struct redis_context* c = (struct redis_context*)r->context;
    uv_mutex_lock(&c->lock);
    if (c->is_closed) {
        uv_mutex_unlock(&c->lock);
        return;
    }
    redisContext* conn = (redisContext*)c->conn;
    r->result = NULL;
    r->err = NULL;
    redisReply* reply = (redisReply*)redisCommand(conn, r->cmd);
    r->result = reply;
    uv_mutex_unlock(&c->lock);
}

static void on_query_complete(uv_work_t* req, int status) {
    struct query_req* r = (struct query_req*)req->data;
    if (r->query_cb && (r->err || r->result)) {
        r->query_cb(r->err, r->result, r->udata);
    }
    if (r->result) { freeReplyObject(r->result); }
    if (r->err) { free(r->err); }
    if (r->cmd) { free(r->cmd); }
    free(r);
    free(req);
}

void redis_wrapper::connect(char* ip, int port,
                            void (*open_cb)(const char* err, void* context,
                                            void* udata),
                            void* udata) {
    _new(uv_work_t, w);
    _new(struct connect_req, r);
    r->ip = _strdup(ip);
    r->port = port;
    r->open_cb = open_cb;
    r->udata = udata;
    w->data = (void*)r;
    uv_queue_work(uv_default_loop(), w, connect_work, on_connect_work_complete);
}

void redis_wrapper::close(void* context) {
    _new(uv_work_t, w);
    w->data = context;
    uv_queue_work(uv_default_loop(), w, close_work, on_close_complete);
}

void redis_wrapper::query(void* context, char* cmd,
                          void (*query_cb)(const char* err, redisReply* result,
                                           void* udata),
                          void* udata) {
    _new(uv_work_t, w);
    _new(struct query_req, r);
    r->context = context;
    r->cmd = _strdup(cmd);
    r->query_cb = query_cb;
    r->udata = udata;
    w->data = (void*)r;
    uv_queue_work(uv_default_loop(), w, query_work, on_query_complete);
}
