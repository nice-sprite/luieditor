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

/*https://en.cppreference.com/w/c/memory/aligned_alloc#:~:text=Also%2C%20this%20function%20is%20not%20supported%20in%20Microsoft%20C%20Runtime%20library%20because%20its%20implementation%20of%20free()%20is%20unable%20to%20handle%20aligned%20allocations%20of%20any%20kind.%20Instead%2C%20MSVC%20provides%20_aligned_malloc%20(to%20be%20freed%20with%20_aligned_free).*/
#if defined(_WIN32) || defined(_WIN64)
#define ALIGNED_ALLOC                                                                                                  \
  _aligned_malloc // apparently MSVC cannot free aligned allocations with their impl of aligned_alloc...
#define ALIGNED_FREE _aligned_free
#else
#define ALIGNED_ALLOC aligned_alloc // other compilers
#define ALIGNED_FREE aligned_free
#endif

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