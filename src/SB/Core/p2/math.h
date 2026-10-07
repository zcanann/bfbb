#ifndef PS2_MATH_H
#define PS2_MATH_H

#define HALF_PI 1.5707964f

// Standard API declarations only; implementations remain in the platform runtime.
#ifdef __cplusplus
extern "C" {
#endif

float sqrtf(float value);
double sqrt(double value);
float atan2f(float y, float x);
float powf(float base, float exponent);
float fabsf(float value);
float floorf(float value);
float ceilf(float value);
float acosf(float value);
float asinf(float value);
float expf(float value);
double atan(double value);
double log(double value);
double fmod(double x, double y);

#ifdef __cplusplus
}
namespace std
{
using ::sqrtf;
using ::sqrt;
using ::atan2f;
using ::powf;
using ::fabsf;
using ::floorf;
using ::ceilf;
using ::acosf;
using ::asinf;
using ::expf;
float atan(float x);
float logf(float x);
}
#endif

#endif
