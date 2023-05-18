#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef float f32;
typedef double f64;

typedef int b32;
typedef bool b8;


#define VECTOR4_PARTS(vec) vec.x, vec.y, vec.z, vec.w
#define VECTOR3_PARTS(vec) vec.x, vec.y, vec.z
#define VECTOR2_PARTS(vec) vec.x, vec.y

#define COPY_XY(dst, src)                                                                                              \
  dst.x = src.x;                                                                                                       \
  dst.y = src.y;

#define COPY_XYZ(dst, src)                                                                                             \
  dst.x = srx.x;                                                                                                       \
  dst.y = src.y;                                                                                                       \
  dst.z = src.z;

#define FLOAT2_SUBTRACT(store, a, b)                                                                                   \
  store.x = a.x - b.x;                                                                                                 \
  store.y = a.y - b.y;  

#define FLOAT2_EQUAL(a, b) (a.x == b.x && a.y == b.y)
