#include "timer.h"
#include "../defines.h"
#include <Windows.h>

Timer::Timer() {
    this->a = 0;
    this->b = 0;
    QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
    this->seconds_per_count = 1.0 / (double)frequency;
}

f64 Timer::elapsed_ms() 
{ 
    return  f64((1000 * this->count_cycles()) * this->seconds_per_count); 
}

u64 Timer::count_cycles()
{
    return u64(this->b - this->a);
}

void Timer::begin()
{ 
    // store current 
    QueryPerformanceCounter((LARGE_INTEGER*)&this->a);
}

void Timer::end()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&this->b);
}



