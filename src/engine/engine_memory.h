
#pragma once
#include <windows.h>

#include "../defines.h"
#include <Tracy.hpp>

/*https://en.cppreference.com/w/c/memory/aligned_alloc#:~:text=Also%2C%20this%20function%20is%20not%20supported%20in%20Microsoft%20C%20Runtime%20library%20because%20its%20implementation%20of%20free()%20is%20unable%20to%20handle%20aligned%20allocations%20of%20any%20kind.%20Instead%2C%20MSVC%20provides%20_aligned_malloc%20(to%20be%20freed%20with%20_aligned_free).*/
#if defined(_WIN32) || defined(_WIN64)
#define ALIGNED_ALLOC                                                                                                  \
  _aligned_malloc // apparently MSVC cannot free aligned allocations with their impl of aligned_alloc...
#define ALIGNED_FREE _aligned_free
#else
#define ALIGNED_ALLOC aligned_alloc // other compilers
#define ALIGNED_FREE aligned_free
#endif

void* engine_alloc(const char* name, u64 size, u64 alloc_type, u64 protection_flags);

void engine_free(void* ptr, const char* name);
