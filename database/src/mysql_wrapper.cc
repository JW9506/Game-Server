#include "mysql_wrapper.h"
#include <uv.h>
#include <mysql.h>
#include "small_alloc.h"

#define _new(type, var)                                                        \
    type* var = (type*)small_alloc(sizeof(type));                              \
    memset(var, 0, sizeof(type))
#define _free(mem) small_free(mem)

struct connect_req {
    char* ip;
    int port;
    char* db_name;
    char* uname;
    char* upwd;
    void (*open_cb)(const char* err, void* context, void* udata);
    char* err;
    void* context;
    void* udata;
};

struct query_req {
    void* context;
    char* sql;
    void (*query_cb)(const char* err, MYSQL_RES* result, void* udata);
    MYSQL_RES* result;
    char* err;
    void* udata;
};

struct mysql_context {
    MYSQL* pConn;
    uv_mutex_t lock;
    int is_closed;
};

static char* __strdup(const char* src) {
    int len = strlen(src) + 1;
    char* dst = (char*)small_alloc(len);
    strcpy(dst, src);
    dst[len] = 0;
    return dst;
}

static void __free_strdup(char* str) { small_free(str); }

static void connect_work(uv_work_t* req) {
    struct connect_req* r = (struct connect_req*)req->data;
    MYSQL* pConn = mysql_init(NULL);
    if (mysql_real_connect(pConn, r->ip, r->uname, r->upwd, r->db_name, r->port,
                           NULL, 0)) {
        _new(struct mysql_context, c);
        c->pConn = pConn;
        uv_mutex_init(&c->lock);
        mysql_query(pConn, "set names utf8");
        r->context = (void*)c;
        r->err = NULL;
    } else {
        r->context = NULL;
        r->err = __strdup(mysql_error(pConn));
    }
}

static void on_connect_work_complete(uv_work_t* req, int status) {
    struct connect_req* r = (struct connect_req*)req->data;
    r->open_cb(r->err, r->context, r->udata);
    if (r->ip) { __free_strdup(r->ip); }
    if (r->db_name) { __free_strdup(r->db_name); }
    if (r->uname) { __free_strdup(r->uname); }
    if (r->upwd) { __free_strdup(r->upwd); }
    if (r->err) { __free_strdup(r->err); }
    _free(r);
    _free(req);
}

static void close_work(uv_work_t* req) {
    struct mysql_context* c = (struct mysql_context*)req->data;
    uv_mutex_lock(&c->lock);
    c->is_closed = 1;
    mysql_close(c->pConn);
    uv_mutex_unlock(&c->lock);
    uv_mutex_destroy(&c->lock);
    _free(c);
}

static void on_close_complete(uv_work_t* req, int status) { _free(req); }

static void query_work(uv_work_t* req) {
    struct query_req* r = (struct query_req*)req->data;
    struct mysql_context* c = (struct mysql_context*)r->context;
    uv_mutex_lock(&c->lock);
    if (c->is_closed) {
        uv_mutex_unlock(&c->lock);
        return;
    }
    MYSQL* pConn = c->pConn;
    r->result = NULL;
    r->err = NULL;
    if (mysql_query(pConn, r->sql) != 0) {
        r->err = __strdup(mysql_error(pConn));
        uv_mutex_unlock(&c->lock);
        return;
    }
    MYSQL_RES* result = mysql_store_result(pConn);
    r->result = result;
    uv_mutex_unlock(&c->lock);
}

static void on_query_complete(uv_work_t* req, int status) {
    struct query_req* r = (struct query_req*)req->data;
    if (r->query_cb && r->result) { r->query_cb(r->err, r->result, r->udata); }
    if (r->result) {
        mysql_free_result(r->result);
        r->result = NULL;
    }
    if (r->err) { __free_strdup(r->err); }
    if (r->sql) { __free_strdup(r->sql); }
    _free(r);
    _free(req);
}

void mysql_wrapper::connect(const char* ip, int port, const char* db_name,
                            const char* uname, const char* pwd,
                            void (*open_cb)(const char* err, void* context,
                                            void* udata),
                            void* udata) {
    _new(uv_work_t, w);
    _new(struct connect_req, r);
    r->ip = __strdup(ip);
    r->port = port;
    r->db_name = __strdup(db_name);
    r->uname = __strdup(uname);
    r->upwd = __strdup(pwd);
    r->open_cb = open_cb;
    r->udata = udata;
    w->data = (void*)r;
    uv_queue_work(uv_default_loop(), w, connect_work, on_connect_work_complete);
}

void mysql_wrapper::close(void* context) {
    _new(uv_work_t, w);
    w->data = context;
    uv_queue_work(uv_default_loop(), w, close_work, on_close_complete);
}

void mysql_wrapper::query(void* context, const char* sql,
                          void (*query_cb)(const char* err, MYSQL_RES* result,
                                           void* udata),
                          void* udata) {
    _new(uv_work_t, w);
    _new(struct query_req, r);
    r->context = context;
    r->sql = __strdup(sql);
    r->query_cb = query_cb;
    r->udata = udata;
    w->data = (void*)r;
    uv_queue_work(uv_default_loop(), w, query_work, on_query_complete);
}
