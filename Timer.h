#ifndef _BOAT_TIMER_
#define _BOAT_TIMER_

class Timer {
public:
    Timer();
    virtual ~Timer();

    unsigned long now();

    bool elapsed(unsigned long milliseconds);

    void reset();

    unsigned long timegap();
    
private:
    unsigned long _start;
    unsigned long _last;
};
#endif
