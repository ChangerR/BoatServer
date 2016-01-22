#include "Hardware.h"
#include <string.h>
#include "util.h"

inline unsigned short parseVersion(const char* version) {
    unsigned short ver = 0,ver2 = 0;
    while(*version <= '9' && *version >= '0') {
        ver = ver * 10 + (*version - '0');
        version++;
    }
    if(*version++ != '.')return 0;
    while(*version <= '9' && *version >= '0') {
        ver2 = ver2 * 10 + (*version - '0');
        version++;
    }
    return (ver & 0x7f) << 8 | (ver2 & 0xff);
}

void HWStatus::setLaserA(int l1,int l2) {
    laser1 = l1;
    laser2 = l1;
    update_flag |= LASERA_UPDATE;
}

void HWStatus::setLaserB(int l3,int l4,int l5) {
    laser3 = l3;
    laser4 = l4;
    laser5 = l5;
    update_flag |= LASERB_UPDATE;
}

void HWStatus::setGPS(float _longitude,float _latitude,float _h,float _s,float _t) {
    longitude = _longitude;
    latitude = _latitude;
    height = _h;
    speed = _s;
    time = _t;
    update_flag |= GPS_UPDATE;
}

void HWStatus::setIMU(float _r,float _p,float _y) {
    roll = _r;
    pitch = _p;
    yaw = _y;
    update_flag |= MPU_UPDATE;
}

bool HWStatus::isAllUpdated() {
    return update_flag == ALL_UPDATE;
}

void HWStatus::reset() {
    timer.reset();
    update_flag = 0;
}

void HWStatus::copyStatus(Status* s) {
    s->timer.reset();
    s->timegap = timer.timegap();
    s->laser1 = laser1;s->laser2 = laser2;
    s->laser3 = laser3;s->laser4 = laser4;
    s->laser5 = laser5;
    s->roll = roll;s->pitch = pitch;
    s->yaw = yaw;
    s->longitude = longitude;s->latitude = latitude;
    s->height = height;s->speed = speed;
    s->time = time;
    s->isUpdated = true;
}

class HWCommand {
public:
    HWCommand(int device,const char* cmd) {
        _cmdLen = strlen(cmd);
        _device = device;
        if(_cmdLen < 63) {
            strcpy(_cmd,cmd);
        } else {
            _cmdLen = 0;
        }
    }
public:
    int _cmdLen;
    char _cmd[64];
    int _device;
};

Hardware::Hardware(const char* serial1,const char* serial2) {
    _serial1 = new Serial(serial1);
    _serial2 = new Serial(serial2);
    _events = new CmdEvent();
    _cmdParser1 = new Command(_events);
    _cmdParser2 = new Command(_events);
    _status = new HWStatus();
    _state = HARDWARE_CLOSED;
    _Version1 = _Version2 = 0;
    pthread_mutex_init(&_lock,NULL);

    _events->addEvent("LaserA",Hardware::LaserARead,_status);
    _events->addEvent("LaserB",Hardware::LaserBRead,_status);
    _events->addEvent("GPS",Hardware::GPSRead,_status);
    _events->addEvent("IMU",Hardware::IMURead,_status);
    _events->addEvent("log",Hardware::LogRead,NULL);
}

Hardware::~Hardware() {
    delete _serial1;
    delete _serial2;
    delete _cmdParser1;
    delete _cmdParser2;
    delete _status;
    _events->release();
    pthread_mutex_destroy(&_lock);
}

bool Hardware::openHardware() {

    bool ret = false;

    do {
        if(_serial1->touchForCDCReset() == false||_serial1->begin(_B115200) == false) {
            printf("Serial 1 Open failed\n");
            break;
        }

        if(_serial2->touchForCDCReset() == false||_serial2->begin(_B115200) == false) {
            printf("Serial 2 Open failed\n");
            break;
        }
        ret = true;
        _state = HARDWARE_OPENING;
    }while(0);

    return ret;
}

void Hardware::closeHardware() {

    _state = HARDWARE_CLOSING;
}

HWStatus* Hardware::getStatus() {
    return _status;
}

CmdEvent* Hardware::getEvents() {
    return _events;
}

void Hardware::flushSendList() {
    pthread_mutex_lock(&_lock);
    for(list<HWCommand*>::node *p = _sendList.begin();p != _sendList.end(); p = p->next) {
        HWCommand* cmd = p->element;

        if(cmd->_device & 0x1) {
            _serial1->write(cmd->_cmd,cmd->_cmdLen);
        }
        if(cmd->_device & 0x2) {
            _serial2->write(cmd->_cmd,cmd->_cmdLen);
        }
        delete cmd;
    }
    _sendList.clear();
    pthread_mutex_unlock(&_lock);
}

int Hardware::processHardwareMsg() {
    char buf[256] = {0};
    switch (_state) {
        case HARDWARE_OPENING:
        {
            if(_Version1 == 0) {
                if(_serial1->available()&&_serial1->readline(buf,256) > 0) {
                    printf("Recv Firmware Version1:%s\n",buf);
                    if(!strncmp(buf,"FWVersion(",10)&&(buf[10] == 'A' || buf[10] == 'B')){
                        _Version1 = parseVersion(buf + 11);

                        if(_Version1) {
                            _Version1 = _Version1 | ((buf[10] - 'A') << 15);
                        }
                        printf("parse Version 1 success: version1=%d\n",_Version1);
                    }
                }else if(_sendTimer1.elapsed(1000)){
                    _serial1->write("Firmware()\n",11);
                    printf("Send Firmware 1\n");
                }
            }

            if(_Version2 == 0) {
                if(_serial2->available()&&_serial2->readline(buf,256) > 0) {
                    printf("Recv Firmware Version2:%s\n",buf);
                    if(!strncmp(buf,"FWVersion(",10)&&(buf[10] == 'A' || buf[10] == 'B')){
                        _Version2 = parseVersion(buf + 11);

                        if(_Version2) {
                            _Version2 = _Version2 | ((buf[10] - 'A') << 15);
                        }
                        printf("parse Version 2 success: version2=%d\n",_Version2);
                    }
                }else if(_sendTimer2.elapsed(1000)){
                    _serial2->write("Firmware()\n",11);
                    printf("Send Firmware 2\n");
                }
            }

            if(_Version1 != 0 && _Version2 != 0) {
                //printf("Parse Hardware version success version1=%d version2=%d\n",_Version1,_Version2);

                int v = (_Version1 >> 15) + (_Version2 >> 15);
                if(v == 1) {
                    if(_Version1 & 0x8000) {
                        int sV = _Version1;
                        Serial* tmps = _serial1;
                        _Version1 = _Version2;
                        _Version2 = sV;
                        _serial1 = _serial2;
                        _serial2 = tmps;
                    }
                    _state = HARDWARE_RUNNING;
                }else {
                    printf("Error Hardware,Maybe Hardware1 same as Hardware2\n");
                    _state = HARDWARE_ERROR;
                }
            }

        }
            break;
        case HARDWARE_RUNNING:
            if(_serial1->available()&&_serial1->readline(buf,256) > 0) {
                _cmdParser1->parseWithFire(buf);
            }
            if(_serial2->available()&&_serial2->readline(buf,256) > 0) {
                _cmdParser2->parseWithFire(buf);
            }
            flushSendList();
            break;
        case HARDWARE_CLOSING:
            flushSendList();
            _serial1->close();
            _serial2->close();
            _state = HARDWARE_CLOSED;
            break;
        case HARDWARE_CLOSED:
            break;
    }

    return _state;
}

void Hardware::requestStatus() {
    HWCommand* cmd = new HWCommand(3,"RequestData()\n");
    pthread_mutex_lock(&_lock);
    _sendList.push_back(cmd);
    pthread_mutex_unlock(&_lock);

    if(_status->isAllUpdated())
        _status->reset();
}

void Hardware::led(int id,int data) {
    char buf[256] = {0};

    sprintf(buf,"Led%d(%d)\n",id,data);
    HWCommand* cmd = new HWCommand(2,buf);
    pthread_mutex_lock(&_lock);
    _sendList.push_back(cmd);
    pthread_mutex_unlock(&_lock);
}

void Hardware::thruster(int id,int data) {
    char buf[256] = {0};

    sprintf(buf,"Thruster%d(%d)\n",id,data);
    HWCommand* cmd = new HWCommand(2,buf);
    pthread_mutex_lock(&_lock);
    _sendList.push_back(cmd);
    pthread_mutex_unlock(&_lock);
}

void Hardware::motor(int id,int data) {
    char buf[256] = {0};

    sprintf(buf,"Motor%d(%d)\n",id,data);
    HWCommand* cmd = new HWCommand(2,buf);
    pthread_mutex_lock(&_lock);
    _sendList.push_back(cmd);
    pthread_mutex_unlock(&_lock);
}

int Hardware::LaserARead(int args,char (*argv)[MAX_CMD_ARGUMENT_LEN],void* user) {
    HWStatus* status = (HWStatus*)user;
    if(args == 2) {
        int laser1data = parse_int_dec(argv[0]);
        int laser2data = parse_int_dec(argv[1]);
        status->setLaserA(laser1data,laser2data);
    }
    return 0;
}

int Hardware::LaserBRead(int args,char (*argv)[MAX_CMD_ARGUMENT_LEN],void* user) {
    HWStatus* status = (HWStatus*)user;
    if(args == 3) {
        int laser3data = parse_int_dec(argv[0]);
        int laser4data = parse_int_dec(argv[1]);
        int laser5data = parse_int_dec(argv[2]);
        status->setLaserB(laser3data,laser4data,laser5data);
    }
    return 0;
}

int Hardware::GPSRead(int args,char (*argv)[MAX_CMD_ARGUMENT_LEN],void* user) {
    HWStatus* status = (HWStatus*)user;
    if(args == 5) {
        float longitudedata = parse_float(argv[0]);
        float latitudedata = parse_float(argv[1]);
        float heightdata = parse_float(argv[2]);
        float speeddata = parse_float(argv[3]);
        float timedata = parse_float(argv[4]);
        status->setGPS(longitudedata,latitudedata,heightdata,speeddata,timedata);
    }
    return 0;
}

int Hardware::IMURead(int args,char (*argv)[MAX_CMD_ARGUMENT_LEN],void* user) {
    HWStatus* status = (HWStatus*)user;
    if(args == 3) {
        float roll = parse_float(argv[0]);
        float pitch = parse_float(argv[1]);
        float yaw = parse_float(argv[2]);
        status->setIMU(roll,pitch,yaw);
    }
    return 0;
}

int Hardware::LogRead(int args,char (*argv)[MAX_CMD_ARGUMENT_LEN],void* user) {
    if(args == 1) {
        printf("Serial Log===>%s\n",argv[0]);
    }
    return 0;
}
