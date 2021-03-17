#include "logger.h"
#include <string>
#include <uv.h>
#include <cstdio>
#include <ctime>

static std::string g_prefix;
static std::string g_log_path;
static uv_fs_t g_file_handle;
static uint32_t g_current_day;
static uint32_t g_last_second;
static char g_format_time[64] = { 0 };
static char* g_log_level[] = { "DEBUG", "WARNING", "ERROR" };
static bool g_std_out;

static void open_file(struct tm* time_struct) {
    int result = 0;
    static char file_name[128] = { 0 };
    if (g_file_handle.result != 0) {
        uv_fs_close(uv_default_loop(), &g_file_handle,
                    (uv_file)g_file_handle.result, NULL);
        uv_fs_req_cleanup(&g_file_handle);
        g_file_handle.result = 0;
    }
    sprintf(file_name, "%s%s.%4d%02d%02d.log", g_log_path.c_str(),
            g_prefix.c_str(), time_struct->tm_year + 1900,
            time_struct->tm_mon + 1, time_struct->tm_mday);
    result = uv_fs_open(NULL, &g_file_handle, file_name,
                        O_CREAT | O_RDWR | O_APPEND, S_IREAD | S_IWRITE, NULL);
    if (result < 0) {
        fprintf(stderr, "open [%s] file failed, reason: %s\n", file_name,
                uv_strerror(result));
    }
}

static void prepare_file() {
    time_t now = time(0);
    now += 8 * 60 * 60;
    struct tm* time_struct = gmtime(&now);
    if (g_file_handle.result == 0) {
        g_current_day = time_struct->tm_mday;
        open_file(time_struct);
    } else {
        if (g_current_day != time_struct->tm_mday) {
            g_current_day = time_struct->tm_mday;
            open_file(time_struct);
        }
    }
}

static void format_time() {
    time_t now = time(0);
    now += 8 * 60 * 60;
    struct tm* time_struct = gmtime(&now);
    if (now != g_last_second) {
        g_last_second = (uint32_t)now;
        memset(g_format_time, 0, sizeof(g_format_time));
        sprintf(g_format_time, "%4d%02d%02d %02d:%02d:%02d ",
                time_struct->tm_year + 1900, time_struct->tm_mon + 1,
                time_struct->tm_mday, time_struct->tm_hour, time_struct->tm_min,
                time_struct->tm_sec);
    }
}

void logger::init(const char* path, const char* prefix, bool std_output) {
    g_prefix = prefix;
    g_log_path = path;
    g_std_out = std_output;
    if (g_log_path.back() != '/') { g_log_path += "/"; }
    std::string tmp_path = g_log_path;

    uv_fs_t req;
    int result;
    size_t find = tmp_path.find("/");
    while (find != -1) {
        result = uv_fs_mkdir(uv_default_loop(), &req,
                             tmp_path.substr(0, find).c_str(), 0755, NULL);
        find = tmp_path.find("/", find + 1);
    }
    uv_fs_req_cleanup(&req);
}

void logger::log(const char* file_name, int line_num, int level,
                 const char* msg, ...) {
    prepare_file();
    format_time();
    static char msg_meta_info[1024] = { 0 };
    static char msg_content[1024 * 10] = { 0 };
    static char new_line = '\n';
    static char space = ' ';
    va_list args;
    va_start(args, msg);
    vsnprintf(msg_content, sizeof(msg_content), msg, args);
    va_end(args);

    sprintf(msg_meta_info, "%s:%u ", file_name, line_num);
    uv_buf_t buf[7];
    buf[0] = uv_buf_init(g_format_time, (uint32_t)strlen(g_format_time));
    buf[1] =
        uv_buf_init(g_log_level[level], (uint32_t)strlen(g_log_level[level]));
    buf[2] = uv_buf_init(&space, 1);
    buf[3] = uv_buf_init(msg_meta_info, (uint32_t)strlen(msg_meta_info));
    buf[4] = uv_buf_init(&new_line, 1);
    buf[5] = uv_buf_init(msg_content, (uint32_t)strlen(msg_content));
    buf[6] = uv_buf_init(&new_line, 1);

    uv_fs_t write_req;
    int result = uv_fs_write(NULL, &write_req, (uv_file)g_file_handle.result,
                             buf, sizeof(buf), 0, NULL);
    if (result < 0) {
        fprintf(stderr, "%s[%s] log failed %s", g_format_time,
                g_log_level[level], g_log_path.c_str());
    }
    uv_fs_req_cleanup(&write_req);
    if (g_std_out) {
        printf("%s:%u\n[%s]%s\n", file_name, line_num, g_log_level[level],
               msg_content);
    }
}
