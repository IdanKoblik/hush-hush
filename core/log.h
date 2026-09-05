#pragma once

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int verbose;

#define INFO(fmt, ...) printf("[+] " fmt "\n", ##__VA_ARGS__)

#define WARN(fmt, ...) fprintf(stderr, "[!] " fmt "\n", ##__VA_ARGS__)

#define ERROR(fmt, ...)                                                                       \
    do {                                                                                      \
        int _err = errno;                                                                     \
        fprintf(stderr, "[-] " fmt " (errno=%d: %s)\n", ##__VA_ARGS__, _err, strerror(_err)); \
    } while (0)

#define DEBUG(fmt, ...)                                                                      \
    do {                                                                                     \
        if (verbose) {                                                                       \
            time_t _now = time(NULL);                                                        \
            struct tm _tm;                                                                   \
            localtime_r(&_now, &_tm);                                                        \
            char _buf[64];                                                                   \
            strftime(_buf, sizeof(_buf), "%Y-%m-%d %H:%M:%S", &_tm);                         \
            printf("[%s %s:%d] [DEBUG] " fmt "\n", _buf, __FILE__, __LINE__, ##__VA_ARGS__); \
        }                                                                                    \
    } while (0)

#ifdef __cplusplus
}
#endif
