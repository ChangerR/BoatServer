#ifndef _BOAT_PILOT_H
#define _BOAT_PILOT_H
#include <pthread.h>
#include "list.h"
#include <string>
#include "json/document.h"
#include "Timer.h"
#include "Hardware.h"
#include "AutoController.h"
#define PILOT_MANUAL        0
#define PILOT_HALFMANUAL    1
#define PILOT_AUTOCONTROL   2

class AutoController;

class Pilot {
public:
    Pilot(Hardware* h,const char* script);
    virtual ~Pilot();

    void pushControl(int id,const char* cmd);
    void cancelControl(int id);

    bool needBroadcast(int* id,char* buf,int* len);

    void handleControl();

	void setAutoControlScript(const char* script);
private:
    void manualControl(rapidjson::Document*,int);
    void halfManualControl(rapidjson::Document*,int);
    void autoControl();
    void resetControl();
    void pushBroadcast(int id,const char* cmd);

    void sendThruster(int id,int device,int power) {

        if(_hardware == NULL)return;
        if(power > 100)power = 100;
        if(power < -100)power = -100;
        int temp = 0;
        switch (device) {
            case 1:
                _thruster1id = id;
                temp = power * 5 + 1500;
                if(temp != _thruster1) {
                    _hardware->thruster(1,temp);
                    _thruster1 = temp;
                }
                break;
            case 2:
                _thruster2id = id;
                temp = power * 5 + 1500;
                if(temp != _thruster2) {
                    _hardware->thruster(2,temp);
                    _thruster2 = temp;
                }
                break;
            case 3:
                _motor1id = id;
                temp = power;
                if(temp != _motor1) {
                    _hardware->motor(1,temp);
                    _motor1 = temp;
                }
                break;
            case 4:
                _motor2id = id;
                temp = power;
                if(temp != _motor2) {
                    _hardware->motor(2,temp);
                    _motor2 = temp;
                }
                break;
            case 5:
                _motor3id = id;
                temp = power;
                if(temp != _motor3) {
                    _hardware->motor(3,temp);
                    _motor3 = temp;
                }
                break;
            case 6:
                _motor4id = id;
                temp = power;
                if(temp != _motor4) {
                    _hardware->motor(4,temp);
                    _motor4 = temp;
                }
                break;
            default:
                break;
        }
    }

    void sendStatus();

    void sendLED(int id,int device,int power) {
        if(_hardware == NULL)return;
        if(power < 0)power = 0;
        if(power > 100)power = 100;

        switch (device) {
            case 1:
                _led1 = power;
                _led1id = id;
                _hardware->led(1,_led1);
                break;
            case 2:
                _led2id = id;
                _led2 = power;
                _hardware->led(2,_led2);
                break;
            default:
                break;
        }
    }

	void cancelIdControl(int id);

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

    int _thruster1id,_thruster2id;
    int _thruster1,_thruster2;
    int _motor1id,_motor2id,_motor3id,_motor4id;
    int _motor1,_motor2,_motor3,_motor4;
    int _led1id,_led2id;
    int _led1,_led2;
    Timer _StatusUpdate;
    AutoController* _autoController;
    friend class AutoController;
    std::string _autoScript;
};
#endif
