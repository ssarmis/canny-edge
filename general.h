#pragma once

#include <stdint.h>
#include <assert.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int16_t i16;
typedef int32_t i32;

typedef float r32;
typedef double r64;

#define MIN(value, min) ((value) = (value) < (min) ? (min) : (value))
#define MAX(value, max) ((value) = (value) > (max) ? (max) : (value))
#define CLAMP(value, min, max) ((value) = (value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

#define ASSERT(x) assert(x)

#define RAD2DEG(x) ((x) * 57.29578)