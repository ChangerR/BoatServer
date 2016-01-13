#ifndef _BOAT_PILOT_H
#define _BOAT_PILOT_H

class Hardware;

class Pilot {
public:
    Pilot(Hardware*);
    virtual ~Pilot();

    void pushControl(int id,const char* cmd);
    void cancelControl(int id);
};
#endif
