#ifndef PS2_STRING_H
#define PS2_STRING_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void* memset(void* destination, int value, size_t size);
size_t strlen(const char* string);

#ifdef __cplusplus
}
#endif

#endif
