#include "serial.h"
#include <string.h>
#ifdef SLSERVER_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include "logger.h"
#include <poll.h>

static int getDCBBuadRate(BUADRATE b) {
	int br = B115200;
	switch(b) {
		case _B9600:
			br = B9600;
			break;
		case _B19200:
			br = B19200;
			break;
		case _B115200:
			br = B115200;
			break;
	}
	return br;
}

inline void copy_iner_buffer(char* buf,int from,int to,int len) {
	for(int i = 0; i < len;i++) {
		buf[to + i] = buf[from + i];
	}
}

Serial::Serial(const char* com) {
	strcpy(_port,com);

	memset(_buffer,0,MAX_SERIALBUFFER_SIZE);

	_wpos = 0;
	_user = NULL;
	_serial_fd = -1;
	_running =false;
}

Serial::~Serial() {
	close();
}

bool Serial::isAlive() {
	return _running;
}

bool Serial::begin(BUADRATE b) {
	struct termios _option;

	Logger::getInstance()->info(5,"[Serial] Open Serial Port:%s",_port);
	_serial_fd = open(_port,O_RDWR|O_NOCTTY|O_NDELAY);

	if(_serial_fd < 0) {
		Logger::getInstance()->error("[Serial] Open Serial Port Failed%d",_serial_fd);
		return false;
	}

	tcgetattr(_serial_fd,&_option);

	_option.c_cflag != (CLOCAL|CREAD);
	_option.c_cflag &= ~CSIZE;
	_option.c_cflag &= ~CRTSCTS;
	_option.c_cflag |= CS8;
	_option.c_cflag &= ~CSTOPB;
	_option.c_iflag |= IGNPAR;
	_option.c_oflag &= ~OPOST;
	_option.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	_option.c_cflag &= ~PARENB;   /* Clear parity enable */
	_option.c_iflag &= ~INPCK;     /* Enable parity checking */

	cfsetospeed(&_option,getDCBBuadRate(b));
	cfsetispeed(&_option,getDCBBuadRate(b));

	tcflush(_serial_fd,TCIOFLUSH);

	if(tcsetattr(_serial_fd, TCSANOW, &_option) != 0) {
		Logger::getInstance()->error("[Serial] setup serial error\n");
		return false;
	}

	_wpos = 0;
	_running = true;

	return true;
}

void Serial::close() {
	_running  = false;
	::close(_serial_fd);
}

int Serial::write(const char* buf,int len) {
	int _len = 0;
	if(!_running)
		return -1;

	_len = ::write(_serial_fd,buf,len);
	if(_len == len) {
        return _len;
    } else {
        tcflush(_serial_fd, TCOFLUSH);
        return -1;
    }
}

int Serial::read() {
	int nRead = MAX_SERIALBUFFER_SIZE - _wpos - 1;
	int len = 0;

	//	printf("serial hardware read enter\n");
	if((len = ::read(_serial_fd,_buffer + _wpos,nRead)) < 0 ) {
		if(errno != EAGAIN)
			Logger::getInstance()->error("[Serial] when read serial occur error errno: %d",errno);
		return -1;
	}
	//printf("read serial data len:%d \n",len);
	_wpos += len;
	//sched_yield();
	_buffer[_wpos] = 0;
	//usleep(1);
	return _wpos;
}

int Serial::read(char* buf,int len) {
	int nRead = -1;

	if(len <= _wpos) {
		memcpy(buf,_buffer,len);
		nRead = len;
		_wpos -= len;
		copy_iner_buffer(_buffer,nRead,0,_wpos);
	}
	return nRead;
}

int Serial::available() {
	//fix #1 replace select to poll which can dectect device error
	//fd_set fds;
	struct pollfd events[1] = {0};
	//struct timeval timeout = {0,0};
	int ret = 0;
	if(!_running)
		return -2;
	
	//FD_ZERO(&fds);
	//FD_SET(_serial_fd,&fds);
	events[0].fd = _serial_fd;
	events[0].events = POLLIN | POLLERR;
	ret = poll((struct pollfd*)&events,1,0);
	//ret = select(_serial_fd + 1,&fds,NULL,NULL,&timeout);
	//if(ret > 0) {
	//	this->read();
	//}else 
	if(ret < 0){
		Logger::getInstance()->error("[Serial] Serial::available() epoll error,code=%d",ret);	
		close();
		return -2;
	}
   
	if(ret != 0) {
		if(events[0].revents & POLLERR) {
			Logger::getInstance()->warning("[Serial] Device occur error,Maybe Device is outline!");
			close();
			return -1;
		}	
		if(events[0].revents & POLLIN) {
			this->read();
		}
	}
	
	return _wpos;
}

int Serial::readline(char* buf,int maxlen) {
	int nRead = 0,nRet = 0;

	for(;nRead < _wpos;nRead++) {
		if(_buffer[nRead] == '\n')
			break;
	}

	if(_buffer[nRead] == '\n' && nRead < _wpos) {

		if(nRead > maxlen) {
			return -1;
		}
		memcpy(buf,_buffer,nRead);
		if (buf[nRead - 1] == '\r') {
			buf[nRead - 1] = 0;
			nRet = nRead - 2;
		}
		else {
			buf[nRead] = 0;
			nRet = nRead - 1;
		}
		nRead += 1;

		//Logger::getInstance()->error("+++info+++ %d,%d\n",nRead,_wpos);
		_wpos -= nRead;
		copy_iner_buffer(_buffer,nRead,0,_wpos);
		//fix serial port read error block

		return nRead - 1;
	}

	return 0;
}

int Serial::print(int n) {
	char buf[32];
	int nWrite = -1;
	sprintf_s(buf,"%d",n);
	nWrite = strlen(buf);
	return write(buf,nWrite);
}

int Serial::print(float f) {
	char buf[32];
	int nWrite = -1;
	sprintf_s(buf,"%f",f);
	nWrite = strlen(buf);
	return write(buf,nWrite);
}

int Serial::print(const char* s,...) {
	char buf[256];
	int nWrite = -1;

	va_list argptr;
    va_start(argptr, s);
    vsprintf(buf,s, argptr);
    va_end(argptr);
	nWrite = strlen(buf);
	return write(buf,nWrite);
}

int Serial::println(const char* s,...) {
	char buf[256];
	int nWrite = -1;

	va_list argptr;
    va_start(argptr, s);
    vsprintf(buf,s, argptr);
    va_end(argptr);
	nWrite = strlen(buf);
	buf[nWrite]='\n';
	return write(buf,nWrite+1);
}


bool Serial::touchForCDCReset() {
	if(begin(_B115200) == false)
		return false;
	int status;

	Logger::getInstance()->info(5,"[Serial] reset serial port");
	ioctl(_serial_fd, TIOCMGET, &status);
	status &= ~TIOCM_DTR;
	ioctl(_serial_fd, TIOCMSET, &status);

	close();
	return true;
}
#endif
