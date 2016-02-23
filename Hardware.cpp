#include "Hardware.h"
#include <string.h>
#include "util.h"
#include <unistd.h>
#include "logger.h"

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

inline bool parseGPS(const char* buffer,float& _longitude,float& _latitude,float& _speed,char* _time) {
	const char* p = buffer;
	bool ret = false;
	char value[64];
    if(*p++  != '$')return ret;
		
	if(!strncmp(p,"GPRMC,",6)) {
		char* v = (char*)_time + 6;
		p += 6;
		while(*p && *p != ','&& *p != '.')*v++ = *p++;
		
		*v = 0;

		while(*p && *p != ',')p++;

		if(*(++p) != 'A') {
						*_time = 0;
			return ret;
		}else
			ret = true;

		p += 2;
		v = value;
		
		while(*p && *p != ',')*v++ = *p++;
		*v = 0;

		_longitude = parse_float(value) / 100;

		if(*(p + 1) != 'N')
			_longitude = -_longitude;				

		p += 3;
		
		v = value;
		while(*p && *p != ',')*v++ = *p++;
		*v = 0;

		_latitude = parse_float(value) / 100;
		if( *(p + 1) != 'E')
			_latitude = -_latitude;
		
		p += 3;
		
		v = value;
		while(*p && *p != ',')*v++ = *p++;
		*v = 0;
		_speed = parse_float(value) / 1.852;

		p++;
		while(*p&&*p != ',')*v++ = *p++;
		p++;

		v = _time;
		while(*p && *p != ',')*v++ = *p++;
	}
	return ret;
}

void HWStatus::setLaserA(int l1,int l2) {
    laser1 = l2;
    laser2 = l1;
    update_flag |= LASERA_UPDATE;
}

void HWStatus::setLaserB(int l3,int l4,int l5) {
    laser3 = l3;
    laser4 = l4;
    laser5 = l5;
    update_flag |= LASERB_UPDATE;
}

void HWStatus::setGPS(const char* buffer) {
	bool ret = parseGPS(buffer,longitude,latitude,speed,time);
	if(ret != true) {
		time[0] = '0';
		time[1] = 0;
		longitude = 0.f;
		latitude = 0.f;
		speed = 0.f;
	}
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
    update_flag = 0;
}

void HWStatus::copyStatus(Status* s) {
    s->timer.reset();
    s->laser1 = laser1;s->laser2 = laser2;
    s->laser3 = laser3;s->laser4 = laser4;
    s->laser5 = laser5;
    s->roll = roll;s->pitch = pitch;
    s->yaw = yaw;
    s->longitude = longitude;s->latitude = latitude;
    s->speed = speed;
	strcpy(s->time,time);
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
            Logger::getInstance()->error("[Hardware] Serial 1 Open failed,Boat Server will exit immediately!");
            break;
        }
		usleep(500);
        if(_serial2->touchForCDCReset() == false||_serial2->begin(_B115200) == false) {
            Logger::getInstance()->error("[Hardware] Serial 2 Open failed,Boat Server will exit immediately!");
            break;
        }
		usleep(500);
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

        if(cmd->_device & 0x1 && _serial1->isAlive()) {
            _serial1->write(cmd->_cmd,cmd->_cmdLen);
        }
        if(cmd->_device & 0x2 && _serial2->isAlive()) {
            _serial2->write(cmd->_cmd,cmd->_cmdLen);
        }
        delete cmd;
    }
    _sendList.clear();
    pthread_mutex_unlock(&_lock);
}

int Hardware::verifyFWVersion(Serial* s,Timer* t) {
	int version = 0;
	char buf[256] = {0};	
	if(s->available() > 0 && s->readline(buf,256) > 0) {
		Logger::getInstance()->info(5,"[Hardware] Recv Firmware Version:%s",buf);
		if(!strncmp(buf,"FWVersion(",10)&&(buf[10] == 'A' || buf[10] == 'B')){
			version = parseVersion(buf + 11);

			if(version) {
				version = version | ((buf[10] - 'A') << 15);
			}
			Logger::getInstance()->info(5,"[Hardware] Parse Version success: version=%d",version);
		}
	}else if(t->elapsed(1000)){
		s->write("Firmware()\n",11);
	}
	
	return version;
}

int Hardware::processHardwareMsg() {
    char buf[256] = {0};
    switch (_state) {
        case HARDWARE_OPENING:
        {
            if(_Version1 == 0) {
				_Version1 = verifyFWVersion(_serial1,&_sendTimer1);               
			}

            if(_Version2 == 0) {
				_Version2 = verifyFWVersion(_serial2,&_sendTimer2);                
			}

            if(_Version1 != 0 && _Version2 != 0) {
                //Logger::getInstance()->error("Parse Hardware version success version1=%d version2=%d\n",_Version1,_Version2);

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
                    Logger::getInstance()->error("[Hardware] Maybe Hardware1 same as Hardware2");
                    _state = HARDWARE_ERROR;
                }
            }

        }
            break;
        case HARDWARE_RUNNING:
			if(_serial1->isAlive()) {
				if(_Version1 == 0) {
					_Version1 = verifyFWVersion(_serial1,&_sendTimer1);
				}else if(_serial1->available() > 0&&_serial1->readline(buf,256) > 0) {
                	_cmdParser1->parseWithFire(buf);
            	}
			}else if(_sendTimer1.elapsed(5000)&&_serial1->begin(_B115200)) {
				_Version1 = 0;
				_cmdParser1->reset();
			}

			if(_serial2->isAlive()) {
				if(_Version2 == 0) {
					_Version2 = verifyFWVersion(_serial2,&_sendTimer2);
				}else if(_serial2->available() > 0&&_serial2->readline(buf,256) > 0) {
                	_cmdParser2->parseWithFire(buf);
            	}
			}else if(_sendTimer2.elapsed(5000)&&_serial2->begin(_B115200)) {
				_Version2 = 0;
				_cmdParser2->reset();
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
    if(args == 1 && argv[0][0] == '$') {
        Logger::getInstance()->info(2,"GPS DATA:%s",argv[0]);
        status->setGPS(argv[0]);
    }
    return 0;
}

int Hardware::IMURead(int args,char (*argv)[MAX_CMD_ARGUMENT_LEN],void* user) {
    HWStatus* status = (HWStatus*)user;
    if(args == 4) {
        float roll = parse_float(argv[1]);
        float pitch = parse_float(argv[2]);
        float yaw = parse_float(argv[3]);
        status->setIMU(roll,pitch,yaw);
    }
    return 0;
}

int Hardware::LogRead(int args,char (*argv)[MAX_CMD_ARGUMENT_LEN],void* user) {
    if(args == 1) {
        Logger::getInstance()->info(2,"[LOGOUT] %s",argv[0]);
    }
    return 0;
}
