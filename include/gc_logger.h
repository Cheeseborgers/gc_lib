//
// Created by fason on 28/10/2025.
//

#ifndef GC_LOGGER_H
#define GC_LOGGER_H

#include <stdbool.h>
#include <stdio.h>

#ifndef NDEBUG
#include <assert.h>
#endif

// Enable colors by default
#ifndef GC_USE_COLORS
#define GC_USE_COLORS 1
#endif

#if GC_USE_COLORS
// POSIX/WINDOWS/ANSI terminals
#define GC_COLOR_FATAL "\x1b[1;41m"
#define GC_COLOR_ERROR "\x1b[31m"
#define GC_COLOR_WARN  "\x1b[33m"
#define GC_COLOR_INFO  "\x1b[32m"
#define GC_COLOR_DEBUG "\x1b[36m"
#define GC_COLOR_TIMER "\x1b[90m"
#define GC_COLOR_TRACE "\x1b[0m"
#define GC_COLOR_RESET "\x1b[0m"
#else
// No color
#define GC_COLOR_FATAL ""
#define GC_COLOR_ERROR ""
#define GC_COLOR_WARN  ""
#define GC_COLOR_INFO  ""
#define GC_COLOR_DEBUG ""
#define GC_COLOR_TIMER ""
#define GC_COLOR_TRACE ""
#define GC_COLOR_RESET ""
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GC_LOG_LEVEL_NONE = 0,
    GC_LOG_LEVEL_FATAL = 1,
    GC_LOG_LEVEL_ERROR = 2,
    GC_LOG_LEVEL_WARN = 3,
    GC_LOG_LEVEL_INFO = 4,
    GC_LOG_LEVEL_DEBUG = 5,
    GC_LOG_LEVEL_TIMER = 6,
    GC_LOG_LEVEL_TRACE = 7
} gc_log_level;

void gc_log_set_level(gc_log_level level);
void gc_log_set_file(FILE *file);
void gc_log_enable_colors(bool enable);
void v_gc_log(gc_log_level level, const char *file, int line, const char *func, const char *fmt, va_list args);
void gc_log(gc_log_level level, const char *file, int line, const char *func, const char *fmt, ...);
void gc_log_no_pos(gc_log_level level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

// ---- Internal helper ----
#ifdef ARDUINO
#define GC_FILE_BASENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#else
#include <string.h>
#define GC_FILE_BASENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

// ---- Log macros ----
#ifdef GC_NO_LOGGING
#define GC_LOG_TRACE(...) ((void)0)
#define GC_LOG_TIMER(...) ((void)0)
#define GC_LOG_DEBUG(...) ((void)0)
#define GC_LOG_INFO(...)  ((void)0)
#define GC_LOG_WARN(...)  ((void)0)
#define GC_LOG_ERROR(...) ((void)0)
#define GC_LOG_FATAL(...) ((void)0)
#elif  defined(NDEBUG)
#define GC_LOG_TRACE(fmt, ...) gc_log(GC_LOG_LEVEL_TRACE, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_TIMER(fmt, ...) gc_log(GC_LOG_LEVEL_TIMER, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_DEBUG(fmt, ...) gc_log(GC_LOG_LEVEL_DEBUG, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_INFO(fmt, ...) gc_log(GC_LOG_LEVEL_INFO, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_WARN(fmt, ...) gc_log(GC_LOG_LEVEL_WARN, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_ERROR(fmt, ...) gc_log(GC_LOG_LEVEL_ERROR, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_FATAL(fmt, ...) gc_log(GC_LOG_LEVEL_FATAL, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#else
#define GC_LOG_TRACE(fmt, ...) gc_log_no_pos(GC_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define GC_LOG_TIMER(fmt, ...) gc_log(GC_LOG_LEVEL_TIMER, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_DEBUG(fmt, ...)  gc_log(GC_LOG_LEVEL_DEBUG, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_INFO(fmt, ...) gc_log(GC_LOG_LEVEL_INFO, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_WARN(fmt, ...) gc_log(GC_LOG_LEVEL_WARN, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_ERROR(fmt, ...) gc_log(GC_LOG_LEVEL_ERROR, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define GC_LOG_FATAL(fmt, ...) gc_log(GC_LOG_LEVEL_FATAL, GC_FILE_BASENAME, __LINE__, __func__, fmt, ##__VA_ARGS__)
#endif

#endif //GC_LOGGER_H