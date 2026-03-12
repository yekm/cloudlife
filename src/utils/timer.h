#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

namespace common
{

#ifdef _WIN32
class Timer
{
public:
    long long counter;
#else
class Timer : public timespec
{
#endif
public:
    Timer();
    Timer restart();

    Timer operator-(const Timer & a);
    Timer & operator-=(const Timer & a);
    Timer operator+(const Timer & a);
    Timer & operator+=(const Timer & a);

    double seconds();
    long unsigned us();
};

} // namespace common

#endif // TIMER_H
