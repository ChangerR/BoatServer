#ifndef __BOATSERVER_HARDWARE_
#define __BOATSERVER_HARDWARE_
#include "serial.h"

class Hardware {
public:
    Hardware();
    virtual ~Hardware();

private:
    Serial* _serial1;
    Serail* _serial2;
};
#endif
