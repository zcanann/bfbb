#ifndef PS2_STDLIB_H
#define PS2_STDLIB_H

#include <types.h>

// Standard runtime declaration; this header supplies no implementation.
#ifdef __cplusplus
extern "C" {
#endif

void exit(int status);
int abs(int value);
int rand(void);
int atoi(const char* string);

double atof(const char* string);
void qsort(void* base, size_t count, size_t size, int (*compare)(const void*, const void*));

#ifdef __cplusplus
}
#endif

#endif
