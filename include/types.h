
#include <stdint.h>

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float   float32;
typedef double  float64;
typedef float32 f32;
typedef float64 f64;

typedef u32 b32;

#define ASSERT(expression) if(!(expression)) *(int *)0 = 0

