#ifndef _BOAT_PILOT_H
#define _BOAT_PILOT_H
#include <pthread.h>
#include "list.h"
#include <string>
#include "json/document.h"
#include "Timer.h"
#include "Hardware.h"

#define PILOT_MANUAL        0
#define PILOT_HALFMANUAL    1
#define PILOT_AUTOCONTROL   2
class Pilot {
public:
    Pilot(Hardware*);
    virtual ~Pilot();

    void pushControl(int id,const char* cmd);
    void cancelControl(int id);

    bool needBroadcast(int* id,char* buf,int* len);

    void handleControl();


private:
    void manualControl(rapidjson::Document*,int);
    void halfManualControl(rapidjson::Document*,int);
    void autoControl();
    void resetControl();
    void pushBroadcast(int id,const char* cmd);

    void sendStatus();
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
    HWStatus::Status* _status;
    Timer _sendStatusTimer;

    int _thruster1,_thruster2;
    int _motor1,_motor2,_motor3,_motor4;
    int _led1,led2;
    Timer _StatusUpdate;
};
#endif
