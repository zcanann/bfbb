#ifndef PS2_STRING_H
#define PS2_STRING_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void* memmove(void* destination, const void* source, size_t size);
void* memcpy(void* destination, const void* source, size_t size);
void* memset(void* destination, int value, size_t size);
int strcmp(const char* lhs, const char* rhs);
int strncmp(const char* lhs, const char* rhs, size_t size);
int stricmp(const char* lhs, const char* rhs);
char* strcat(char* destination, const char* source);
char* strcpy(char* destination, const char* source);
char* strncpy(char* destination, const char* source, size_t size);
size_t strlen(const char* string);
char* strstr(const char* string, const char* substring);

#ifdef __cplusplus
}
namespace std
{
using ::strstr;
}
#endif

#endif
