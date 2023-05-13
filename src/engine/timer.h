#pragma once
#include "../defines.h"

struct Timer
{
    __int64 frequency;
    __int64 a;
    __int64 b;
    __int64 paused;
    double seconds_per_count;

    Timer();

    f64 elapsed_ms(); // return time since begin() 
    u64 count_cycles(); // returns CPU cycles since begin()
                        
    // put the thing you want to measure between begin and end
    void begin(); // returns begin time
    void end();   // returns end time
};
