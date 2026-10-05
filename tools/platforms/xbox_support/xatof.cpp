// Diagnostic host dependency, copied from the existing xMath.cpp definition.
// This wrapper and the host CRT are excluded from Xbox matching coverage.
#include <stdlib.h>

float xatof(const char* x)
{
    return atof(x);
}
