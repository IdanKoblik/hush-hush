#ifndef LOG_H_
#define LOG_H_

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

extern int verbose;

#define LOG_LEVEL(level, fmt, ...)                                                        \
    do {                                                                                  \
        time_t _now = time(NULL);                                                         \
        struct tm _tm;                                                                    \
        localtime_r(&_now, &_tm);                                                         \
        char _buf[64];                                                                    \
        strftime(_buf, sizeof(_buf), "%Y-%m-%d %H:%M:%S", &_tm);                          \
        fprintf(stderr, "[%s %s:%d] [%s] " fmt "\n",                                     \
                _buf, __FILE__, __LINE__, level, ##__VA_ARGS__);                          \
    } while (0)

#define LOG(fmt, ...)                                                                     \
    do {                                                                                  \
        time_t _now = time(NULL);                                                         \
        struct tm _tm;                                                                    \
        localtime_r(&_now, &_tm);                                                         \
        char _buf[64];                                                                    \
        strftime(_buf, sizeof(_buf), "%Y-%m-%d %H:%M:%S", &_tm);                          \
        fprintf(stderr, "[%s %s:%d] " fmt "\n",                                           \
                _buf, __FILE__, __LINE__, ##__VA_ARGS__);                                \
    } while (0)

#define INFO(fmt, ...) \
    LOG_LEVEL("INFO", fmt, ##__VA_ARGS__)

#define ERROR(fmt, ...)                                                                   \
    do {                                                                                  \
        int _err = errno;                                                                 \
        time_t _now = time(NULL);                                                         \
        struct tm _tm;                                                                    \
        localtime_r(&_now, &_tm);                                                         \
        char _buf[64];                                                                    \
        strftime(_buf, sizeof(_buf), "%Y-%m-%d %H:%M:%S", &_tm);                          \
        fprintf(stderr, "[%s %s:%d] [ERROR] " fmt " (errno=%d: %s)\n",                    \
                _buf, __FILE__, __LINE__, ##__VA_ARGS__, _err, strerror(_err));           \
    } while (0)

#define WARN(fmt, ...) \
    LOG_LEVEL("WARN", fmt, ##__VA_ARGS__)

#define DEBUG(fmt, ...) \
    do { \
        if (verbose) \
            LOG_LEVEL("DEBUG", fmt, ##__VA_ARGS__); \
    } while (0)

#endif // LOG_H_
