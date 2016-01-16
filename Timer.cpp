#include "Timer.h"
#include <time.h>
#include <sys/time.h>

Timer::Timer() {
    reset();
}

Timer::~Timer(){}

unsigned long Timer::now() {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

bool Timer::elapsed(unsigned long milliseconds){
    if (now() - _last > milliseconds) {
        _last = now();
        return true;
    }
    return false;
}

void Timer::reset() {
    _start = now();
    _last = _start;
}

unsigned long Timer::timegap() {
    return now() - _last;
}
