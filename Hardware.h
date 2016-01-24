#ifndef __BOATSERVER_HARDWARE_
#define __BOATSERVER_HARDWARE_
#include "serial.h"
#include "Command.h"
#include "list.h"
#include <string>
#include <pthread.h>
#include "Timer.h"

#define LASERA_UPDATE 1
#define LASERB_UPDATE 2
#define MPU_UPDATE    4
#define GPS_UPDATE    8
#define ALL_UPDATE    15

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

    void setLaserA(int l1,int l2);

    void setLaserB(int l3,int l4,int l5);

    void setGPS(float _longitude,float _latitude,float _h,float _s,float _t);

    void setIMU(float _r,float _p,float _y);

    bool isAllUpdated();

    void reset();

    class Status {
    public:
        Status(){
            laser1 = laser2 = laser3 = 0;
            laser4 = laser5 = 0;
            roll = pitch = yaw = 0.f;
            longitude = latitude = 0.f;
            height = speed = time = 0.f;
        }
        virtual ~Status(){}

        int laser1,laser2,laser3;
        int laser4,laser5;
        float roll,pitch,yaw;
        float longitude,latitude;
        float height,speed,time;
        unsigned long timegap;
        Timer timer;
        bool isUpdated;
    };

    void copyStatus(Status* s);
public:
    int laser1,laser2,laser3;
    int laser4,laser5;
    float roll,pitch,yaw;
    float longitude,latitude;
    float height,speed,time;
    unsigned int update_flag;
    Timer timer;
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

    static int LaserARead(int,char (*)[MAX_CMD_ARGUMENT_LEN],void*);
    static int LaserBRead(int,char (*)[MAX_CMD_ARGUMENT_LEN],void*);
    static int GPSRead(int,char (*)[MAX_CMD_ARGUMENT_LEN],void*);
    static int IMURead(int,char (*)[MAX_CMD_ARGUMENT_LEN],void*);
    static int LogRead(int,char (*)[MAX_CMD_ARGUMENT_LEN],void*);

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
    Timer _sendTimer1,_sendTimer2;
};
#endif
