#ifndef _BOAT_PILOT_H
#define _BOAT_PILOT_H
#include <pthread.h>
#include "list.h"
#include <string>

class Hardware;

#define PILOT_MANUAL        0
#define PILOT_HALFMANUAL    1
#define PILOT_AUTOCONTROL   2
class Pilot {
public:
    Pilot(Hardware*);
    virtual ~Pilot();

    void pushControl(int id,const char* cmd);
    void cancelControl(int id);

    bool needBroadcast(int* id,char* buf);

    void handleControl();
private:
    struct _Cmd {
        int id;
        std::string cmd;
    };
    Hardware* _hardware;
    list<_Cmd*> _RecvList;
    list<_Cmd*> _SendList;
    pthread_mutex_t _RecvLock;
    pthread_mutex_t _SendLock;
    int _PilotState;
};
#endif
