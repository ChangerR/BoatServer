#ifndef __BOATSERVER_HARDWARE_
#define __BOATSERVER_HARDWARE_
#include "serial.h"
#include "Command.h"
#include "list.h"
#include <string>
#include <pthread.h>

#define LASER1_UPDATE 1
#define LASER2_UPDATE 2
#define LASER3_UPDATE 4
#define LASER4_UPDATE 8
#define LASER5_UPDATE 16
#define MPU_UPDATE    32
#define GPS_UPDATE    64
#define ALL_LASER_UPDATE 31
#define ALL_UPDATE    127

class HWStatus {
public:
    HWStatus(){
        laser1 = laser2 = laser3 = 0;
        laser4 = laser5 = 0;
        roll = pitch = yaw = 0.f;
        longitude = latitude = 0.f;
        height = speed = time = 0.f;
        update_flag = 0;
    }
    virtual ~HWStatus(){}

public:
    int laser1,laser2,laser3;
    int laser4,laser5;
    float roll,pitch,yaw;
    float longitude,latitude;
    float height,speed,time;
    unsigned int update_flag;
};

#define HARDWARE_OPENING    1
#define HARDWARE_RUNNING    2
#define HARDWARE_CLOSING    3
#define HARDWARE_CLOSED     4
#define HARDWARE_ERROR      5

class HWCommand;
class Hardware {
public:
    Hardware(const char* serial1,const char* serial2);

    virtual ~Hardware();

    bool openHardware();

    void closeHardware();

    int processHardwareMsg();

    HWStatus* getStatus();

    CmdEvent* getEvents();

    void requestStatus();

    void led(int id,int data);

    void thruster(int id,int data);

    void motor(int id,int data);
private:
    void flushSendList();

    Serial* _serial1;
    Serial* _serial2;
    Command* _cmdParser1;
    Command* _cmdParser2;
    CmdEvent* _events;
    HWStatus* _status;
    int _state;
    unsigned short _Version1;
    unsigned short _Version2;
    list<HWCommand*> _sendList;
    pthread_mutex_t _lock;
};
#endif
