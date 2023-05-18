#include "engine_memory.h"
// seems to have 0 effect on perf, so ill just default to malloc
#define USE_VIRTUALALLOC    1
#define USE_MALLOC          0
#define USE_ALIGNED_MALLOC  0
#define USE_NEW             0

void* engine_alloc(const char* name, u64 size, u64 alloc_type, u64 protection_flags)
{ 
#if USE_VIRTUALALLOC == 1
    void* ptr = ::VirtualAlloc(0, size, alloc_type, protection_flags);
#endif

#if USE_MALLOC == 1
    void* ptr = std::malloc(size);
#endif

#if USE_ALIGNED_MALLOC == 1
    void* ptr = ALIGNED_ALLOC(size, 64) ;
#endif
    
    TracyAllocN(ptr, size, name);
    return ptr;
}

void engine_free(void* ptr, const char* name) 
{
#if USE_VIRTUALALLOC == 1
    VirtualFree(ptr, 0, MEM_RELEASE);
#endif

#if USE_MALLOC == 1
    std::free(ptr);
#endif

#if USE_ALIGNED_MALLOC == 1
    ALIGNED_FREE(ptr);
#endif

    TracyFreeN(ptr, name);
}

