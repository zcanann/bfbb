#ifndef PS2_STDIO_H
#define PS2_STDIO_H

#include "reent.h"

typedef struct __sFILE FILE;
#define stdin (_impure_ptr->_stdin)
#define stdout (_impure_ptr->_stdout)
#define stderr (_impure_ptr->_stderr)

// Standard runtime declarations; no replacement implementations.
#ifdef __cplusplus
extern "C" {
#endif

int fprintf(FILE* stream, const char* format, ...);
int printf(const char* format, ...);
int sprintf(char* buffer, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
