#include "timer.h"
#include <time.h>

namespace common
{

Timer::Timer()
{
    restart();
}

Timer Timer::restart()
{
    Timer tmp = *this;
    clock_gettime( CLOCK_MONOTONIC_RAW, this );
    return tmp;
}


Timer Timer::operator-(const Timer & a)
{
    Timer t(*this);
    return t -= a;
}

Timer & Timer::operator-=(const Timer & a)
{
    tv_sec -= a.tv_sec;
    tv_nsec -= a.tv_nsec;
    if(tv_nsec < 0)
    {
        --tv_sec;
        tv_nsec += 1e9;
    }
    return *this;
}

Timer Timer::operator+(const Timer & a)
{
    Timer t(*this);
    return t += a;
}

Timer & Timer::operator+=(const Timer & a)
{
    tv_sec += a.tv_sec;
    tv_nsec += a.tv_nsec;
    if(tv_nsec < 1e9)
    {
        ++tv_sec;
        tv_nsec -= 1e9;
    }
    return *this;
}


} // namespace common
