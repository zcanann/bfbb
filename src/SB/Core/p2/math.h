#ifndef PS2_MATH_H
#define PS2_MATH_H

// Standard API declarations only; implementations remain in the platform runtime.
#ifdef __cplusplus
extern "C" {
#endif

float sqrtf(float value);
float atan2f(float y, float x);
float powf(float base, float exponent);
float fabsf(float value);
float floorf(float value);

#ifdef __cplusplus
}
namespace std
{
using ::sqrtf;
using ::atan2f;
using ::powf;
using ::fabsf;
using ::floorf;
}
#endif

#endif
