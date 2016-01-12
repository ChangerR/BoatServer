#include "Hardware.h"
#include <string.h>

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
                    }
                }else{
                    _serial1->write("Firmware()\n",11);
                }
            }

            if(_Version2 == 0) {
                if(_serial2->available()&&_serial2->readline(buf,256) > 0) {
                    printf("Recv Firmware Version1:%s\n",buf);
                    if(!strncmp(buf,"FWVersion(",10)&&(buf[10] == 'A' || buf[10] == 'B')){
                        _Version2 = parseVersion(buf + 11);

                        if(_Version2) {
                            _Version2 = _Version2 | ((buf[10] - 'A') << 15);
                        }
                    }
                }else{
                    _serial1->write("Firmware()\n",11);
                }
            }

            if(_Version1 != 0 && _Version2 != 0) {
                int v = (_Version1 >> 15) + (_Version2 >> 15);
                if(v == 1) {
                    if(_Version1 & 0x8000) {
                        int sV = _Version1;
                        Serial* tmps = _serial1;
                        _Version1 = _Version2;
                        _Version2 = sV;
                        _serial1 = _serial2;
                        _serial2 = tmps;
                        _state = HARDWARE_RUNNING;
                    }
                }else{
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
    HWCommand* cmd = new HWCommand(3,"requestData()\n");
    pthread_mutex_lock(&_lock);
    _sendList.push_back(cmd);
    pthread_mutex_unlock(&_lock);
}

void Hardware::led(int id,int data) {
    char buf[256] = {0};

    sprintf(buf,"led%d(%d)\n",id,data);
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
