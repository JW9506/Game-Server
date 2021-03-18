#include "mysql_wrapper.h"
#include <uv.h>
#include <mysql.h>

#define _new(type, var) type* var = (type*)calloc(1, sizeof(type))

struct connect_req {
    char* ip;
    int port;
    char* db_name;
    char* uname;
    char* upwd;
    void (*open_cb)(const char* err, void* context);
    char* err;
    void* context;
};

struct query_req {
    void* context;
    char* sql;
    void (*query_cb)(const char* err,
                     std::vector<std::vector<std::string>>& result);
    std::vector<std::vector<std::string>>* result;
    char* err;
};

struct mysql_context {
    MYSQL* pConn;
    uv_mutex_t lock;
    int is_closed;
};

static void connect_work(uv_work_t* req) {
    struct connect_req* r = (struct connect_req*)req->data;
    MYSQL* pConn = mysql_init(NULL);
    if (mysql_real_connect(pConn, r->ip, r->uname, r->upwd, r->db_name, r->port,
                           NULL, 0)) {
        _new(struct mysql_context, c);
        c->pConn = pConn;
        uv_mutex_init(&c->lock);
        r->context = (void*)c;
        r->err = NULL;
    } else {
        r->context = NULL;
        r->err = _strdup(mysql_error(pConn));
    }
    int ret = mysql_query(pConn, "set names utf8");
    if (ret != 0) { printf("%s\n", mysql_error(pConn)); }
}

static void on_connect_work_complete(uv_work_t* req, int status) {
    struct connect_req* r = (struct connect_req*)req->data;
    r->open_cb(r->err, r->context);
    if (r->ip) { free(r->ip); }
    if (r->db_name) { free(r->db_name); }
    if (r->uname) { free(r->uname); }
    if (r->upwd) { free(r->upwd); }
    if (r->err) { free(r->err); }
    free(r);
    free(req);
}

static void close_work(uv_work_t* req) {
    struct mysql_context* c = (struct mysql_context*)req->data;
    uv_mutex_lock(&c->lock);
    c->is_closed = 1;
    mysql_close(c->pConn);
    uv_mutex_unlock(&c->lock);
    uv_mutex_destroy(&c->lock);
    free(c);
}

static void on_close_complete(uv_work_t* req, int status) { free(req); }

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
        r->err = _strdup(mysql_error(pConn));
        uv_mutex_unlock(&c->lock);
        return;
    }
    MYSQL_RES* result = mysql_store_result(pConn);
    if (!result) {
        uv_mutex_unlock(&c->lock);
        return;
    }
    int num_fields = (int)mysql_num_fields(result);
    r->result = new std::vector<std::vector<std::string>>;
    int line_num = mysql_field_count(pConn);
    MYSQL_ROW row;
    while (row = mysql_fetch_row(result)) {
        r->result->emplace_back();
        for (int i = 0; i < num_fields; ++i) {
            r->result->back().push_back(row[i]);
        }
    }
    mysql_free_result(result);
    uv_mutex_unlock(&c->lock);
}

static void on_query_complete(uv_work_t* req, int status) {
    struct query_req* r = (struct query_req*)req->data;
    if (r->query_cb && r->result) { r->query_cb(r->err, *r->result); }
    if (r->result) { delete r->result; }
    if (r->err) { free(r->err); }
    if (r->sql) { free(r->sql); }
    free(r);
    free(req);
}

void mysql_wrapper::connect(char* ip, int port, char* db_name, char* uname,
                            char* pwd,
                            void (*open_cb)(const char* err, void* context)) {
    _new(uv_work_t, w);
    _new(struct connect_req, r);
    r->ip = _strdup(ip);
    r->port = port;
    r->db_name = _strdup(db_name);
    r->uname = _strdup(uname);
    r->upwd = _strdup(pwd);
    r->open_cb = open_cb;
    w->data = (void*)r;
    uv_queue_work(uv_default_loop(), w, connect_work, on_connect_work_complete);
}

void mysql_wrapper::close(void* context) {
    _new(uv_work_t, w);
    w->data = context;
    uv_queue_work(uv_default_loop(), w, close_work, on_close_complete);
}

void mysql_wrapper::query(
    void* context, char* sql,
    void (*query_cb)(const char* err,
                     std::vector<std::vector<std::string>>& result)) {
    _new(uv_work_t, w);
    _new(struct query_req, r);
    r->context = context;
    r->sql = _strdup(sql);
    r->query_cb = query_cb;
    w->data = (void*)r;
    uv_queue_work(uv_default_loop(), w, query_work, on_query_complete);
}
