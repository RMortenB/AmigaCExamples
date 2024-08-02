#ifndef CORETYPES_H_INCLUDED
#define CORETYPES_H_INCLUDED

#include <stdint.h>
#include <math.h>

typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;

#ifndef CORE_PI
# define CORE_PI 3.1415927f
#endif

#define UNION_CAST(x, destType) (((union {__typeof__(x) a; destType b;})x).b)

static inline float core_rsqrt2(float x)
{
    uint32_t u = UNION_CAST(x, int);
    u = 0x5F1FFFF9ul - (u >> 1);
    return 0.703952253f * UNION_CAST(u, float) * (2.38924456f - x * UNION_CAST(u, float) * UNION_CAST(u, float));
}

static inline float core_rsqrtf(float num)
{
	const float threehalfs = 1.5F;
	union
	{
		int i;
		float f;
	} val;

	float x2 = num * 0.5F;
	val.f = num;
	val.i  = 0x5f3759df - (val.i >> 1);
	num  = val.f;
	num  = num * (threehalfs - (x2 * num * num));   // 1st iteration
	num  = num * (threehalfs - (x2 * num * num));   // 2nd iteration
	num  = num * (threehalfs - (x2 * num * num));   // 3rd iteration

	return num;
}

static inline float core_sqrtf(float num)
{
	return core_rsqrtf(num) * num;
}

static float core_atan2f(float y, float x)
{
	return x>=y ?	(x>=-y?      atanf(y/x):     -CORE_PI/2-atanf(x/y)):
					(x>=-y? CORE_PI/2-atanf(x/y):y>=0? CORE_PI  +atanf(y/x):
					-CORE_PI  +atanf(y/x));
}



#endif