#ifndef GC_TIME_H
#define GC_TIME_H

#ifndef GCDEF
#define GCDEF static inline
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
struct timespec {
    time_t tv_sec;
    long tv_nsec;
};
#define CLOCK_MONOTONIC 0
GCDEF int clock_gettime(int, struct timespec *spec) {
    static LARGE_INTEGER freq;
    static BOOL freq_set = FALSE;
    LARGE_INTEGER count;
    if (!freq_set) {
        QueryPerformanceFrequency(&freq);
        freq_set = TRUE;
    }
    QueryPerformanceCounter(&count);
    spec->tv_sec = (time_t)(count.QuadPart / freq.QuadPart);
    spec->tv_nsec = (long)((count.QuadPart % freq.QuadPart) * 1e9 / freq.QuadPart);
    return 0;
}
#else
#include <time.h>
#endif

GCDEF void get_timestamp(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", &tm_info);
}

GCDEF struct timespec get_time_monotonic() {
    struct timespec ts = {0};
    int ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    assert(ret == 0);
    return ts;
}


GCDEF double delta_secs(struct timespec begin, struct timespec end) {
    const double a = (double)begin.tv_sec + (double)begin.tv_nsec*1e-9;
    const double b = (double)end.tv_sec + (double)end.tv_nsec*1e-9;
    return b - a;
}

#endif //GC_TIME_H