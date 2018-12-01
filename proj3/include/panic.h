#ifndef __PANIC_H__
#define __PANIC_H__
#include <stdio.h>

#define PANIC(msg,...) do{\
    fprintf(stderr, "PANIC: [%s:%d in %s] " msg "\n", \
        __FILE__, __LINE__, __func__ , ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
}while(0)
#endif /* __PANIC_H__ */
