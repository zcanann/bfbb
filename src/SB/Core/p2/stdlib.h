#ifndef PS2_STDLIB_H
#define PS2_STDLIB_H

// Standard runtime declaration; this header supplies no implementation.
#ifdef __cplusplus
extern "C" {
#endif

void exit(int status);
int abs(int value);
int atoi(const char* string);

double atof(const char* string);

#ifdef __cplusplus
}
#endif

#endif
