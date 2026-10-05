#ifndef PS2_STRING_H
#define PS2_STRING_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy(void* destination, const void* source, size_t size);
void* memset(void* destination, int value, size_t size);
int strcmp(const char* lhs, const char* rhs);
char* strcpy(char* destination, const char* source);
char* strncpy(char* destination, const char* source, size_t size);
size_t strlen(const char* string);

#ifdef __cplusplus
}
#endif

#endif
