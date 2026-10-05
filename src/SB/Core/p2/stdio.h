#ifndef PS2_STDIO_H
#define PS2_STDIO_H

// Standard runtime declarations; no replacement implementations.
#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* format, ...);
int sprintf(char* buffer, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
