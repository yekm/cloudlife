#include "timer.h"
#include <time.h>

namespace common
{

#ifdef _WIN32
static long long get_frequency() {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return freq.QuadPart;
}

static long long get_counter() {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count.QuadPart;
}
#endif

Timer::Timer()
{
    restart();
}

Timer Timer::restart()
{
    Timer tmp = *this;
#ifdef _WIN32
    counter = get_counter();
#else
    clock_gettime( CLOCK_MONOTONIC_RAW, this );
#endif
    return tmp;
}


Timer Timer::operator-(const Timer & a)
{
    Timer t(*this);
    return t -= a;
}

Timer & Timer::operator-=(const Timer & a)
{
#ifdef _WIN32
    counter -= a.counter;
#else
    tv_sec -= a.tv_sec;
    tv_nsec -= a.tv_nsec;
    if(tv_nsec < 0)
    {
        --tv_sec;
        tv_nsec += 1e9;
    }
#endif
    return *this;
}

Timer Timer::operator+(const Timer & a)
{
    Timer t(*this);
    return t += a;
}

Timer & Timer::operator+=(const Timer & a)
{
#ifdef _WIN32
    counter += a.counter;
#else
    tv_sec += a.tv_sec;
    tv_nsec += a.tv_nsec;
    if(tv_nsec < 1e9)
    {
        ++tv_sec;
        tv_nsec -= 1e9;
    }
#endif
    return *this;
}

double Timer::seconds() {
#ifdef _WIN32
    return (double)counter / get_frequency();
#else
    return tv_sec + (double)tv_nsec/1e9;
#endif
}

long unsigned Timer::us() {
#ifdef _WIN32
    return (long unsigned)((counter * 1000000LL) / get_frequency());
#else
    return tv_sec*1e9 + tv_nsec;
#endif
}


} // namespace common
