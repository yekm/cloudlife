#ifndef TIMER_H
#define TIMER_H

#include <time.h>

namespace common
{

class Timer : public timespec
{
public:
    Timer();
    Timer restart();

    Timer operator-(const Timer & a);
    Timer & operator-=(const Timer & a);
    Timer operator+(const Timer & a);
    Timer & operator+=(const Timer & a);

    double seconds() {
        return tv_sec + (double)tv_nsec/1e9;
    };
    long unsigned us() {
        return tv_sec*1e9 + tv_nsec;
    };
};

} // namespace common

#endif // TIMER_H
