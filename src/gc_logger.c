// logger.c
//
// Created by fason on 16/10/2025.

#include "gc_logger.h"

#include <stdarg.h>
#include <stdlib.h>

#include "gc_time.h"

#define TIMESTAMP_LEN 20

static gc_log_level g_log_level = GC_LOG_LEVEL_TRACE;
static FILE *g_log_file = NULL;

void gc_log_set_level(gc_log_level level) { g_log_level = level; }

void gc_log_set_file(FILE *file) { g_log_file = file; }

void v_gc_log(gc_log_level level, const char *file, int line,
                 const char *func, const char *fmt, va_list args)
{
    if (level > g_log_level) return;

    char ts[TIMESTAMP_LEN];
    get_timestamp(ts, sizeof(ts));

    const char *level_str;
    const char *color = GC_COLOR_RESET;
    const char *reset = GC_COLOR_RESET;

    switch (level) {
        case GC_LOG_LEVEL_FATAL: color = GC_COLOR_FATAL; level_str = "FATAL"; break;
        case GC_LOG_LEVEL_ERROR: color = GC_COLOR_ERROR; level_str = "ERROR"; break;
        case GC_LOG_LEVEL_WARN:  color = GC_COLOR_WARN;  level_str = "WARN"; break;
        case GC_LOG_LEVEL_INFO:  color = GC_COLOR_INFO;  level_str = "INFO"; break;
        case GC_LOG_LEVEL_DEBUG: color = GC_COLOR_DEBUG; level_str = "DEBUG"; break;
        case GC_LOG_LEVEL_TIMER: color = GC_COLOR_TIMER; level_str = "TIMER"; break;
        case GC_LOG_LEVEL_TRACE: color = GC_COLOR_TRACE; level_str = "TRACE"; break;
        default: return;
    }

    FILE *out = (g_log_file != NULL) ? g_log_file : stderr;

    // Print prefix
    if (line == -1 || func == NULL || file == NULL)
        fprintf(out, "%s[%s] [%s]", color, ts, level_str);
    else
        fprintf(out, "%s[%s] [%s] (%s:%d:%s) ", color, ts, level_str, file, line, func);

    // Print message
    vfprintf(out, fmt, args);
    fprintf(out, "%s\n", reset);

    if (level == GC_LOG_LEVEL_FATAL) fflush(out);
}

void gc_log(gc_log_level level, const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    v_gc_log(level, file, line, func, fmt, args);
    va_end(args);
}

void gc_log_no_pos(gc_log_level level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    v_gc_log(level, NULL, -1, NULL, fmt, args);
    va_end(args);
}
