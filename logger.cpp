#include "logger.h"
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define NONE         "\033[m" 
#define RED          "\033[0;32;31m" 
#define LIGHT_RED    "\033[1;31m" 
#define GREEN        "\033[0;32;32m" 
#define LIGHT_GREEN  "\033[1;32m" 
#define BLUE         "\033[0;32;34m" 
#define LIGHT_BLUE   "\033[1;34m" 
#define DARY_GRAY    "\033[1;30m" 
#define CYAN         "\033[0;36m" 
#define LIGHT_CYAN   "\033[1;36m" 
#define PURPLE       "\033[0;35m" 
#define LIGHT_PURPLE "\033[1;35m" 
#define BROWN        "\033[0;33m" 
#define YELLOW       "\033[1;33m" 
#define LIGHT_GRAY   "\033[0;37m" 
#define WHITE        "\033[1;37m"

char Logger::_instanceFileName[256] = {0};
Logger* Logger::_instance = NULL;

Logger::Logger(const char* filename) {
	memset(_filename,0,256);
	if(filename != NULL) {
		strncpy(_filename,filename,256);
	}	
	_file = NULL;
	_buffer = new char[1024];
	memset(_buffer,0,1024);
	_level = 1;
}

Logger::~Logger() {
	if(_file != NULL && _file != stdout) {
		fflush(_file);
		fclose(_file);
	}
	if(_buffer != NULL)
		delete[] _buffer;
}

bool Logger::init() {
	bool ret = false;

	do {
		if(_filename[0] == 0) {
			_file = stdout;
			ret = true;
		}else{
			_file = fopen(_filename,"a");
			if(_file != NULL)
				ret = true;
		}	
	} while(0);

	return ret;
}

Logger* Logger::getInstance() {
	if(_instance != NULL) {
		return _instance;
	}else{
		_instance = new Logger(_instanceFileName);
		if(_instance->init()){
			return _instance;
		}else {
			return NULL;
		}
	}
}

void Logger::setInstaceFile(const char* filename) {
	if(filename != NULL) {
		strncpy(_instanceFileName,filename,256);
	}	
}

void Logger::destoryInstance() {
	if(_instance != NULL) {
		delete _instance;
		_instance = NULL;
	}
}

struct LogFMT {
	const char* describe;
	const char* color;
}logfmt[] = {
	{"INFO",GREEN},
	{"WARNING",YELLOW},
	{"ERROR",RED},
};

inline void print_head(FILE* f,int i) {
	time_t now;
	struct tm *tm_now;
	char buf[64];
	time(&now);
	tm_now = localtime(&now);
	strftime(buf,64,"%a %b %d %X %Y",tm_now);
	if(f == stdout)
		fprintf(f,"%s%-8.7s" LIGHT_CYAN "%s%s ",logfmt[i].color,logfmt[i].describe,buf,logfmt[i].color);
	else
		fprintf(f,"%-8.7s%s => ",logfmt[i].describe,buf);

}

void Logger::setLogLevel(int l) {
	_level = l;
}

void Logger::info(int level,const char* fmt,...) {
	if(level >= _level) {
		const char*p;
		print_head(_file,0);
		va_list arg;

		va_start(arg,fmt);
		vsprintf(_buffer,fmt,arg);
		va_end(arg);
		
		p = (_file == stdout ? NONE "\n" : "\n");
		strcat(_buffer,p);
		fwrite(_buffer,strlen(_buffer),1,_file);
	}
}

void Logger::warning(const char* fmt,...) {
	print_head(_file,1);
	va_list arg;
	const char* p;
	va_start(arg,fmt);
	vsprintf(_buffer,fmt,arg);
	va_end(arg);

	p = _file == stdout ? NONE "\n" : "\n";
	strcat(_buffer,p);
	fwrite(_buffer,strlen(_buffer),1,_file);
}

void Logger::error(const char* fmt,...) {
	print_head(_file,2);
	va_list arg;
	const char* p;
	va_start(arg,fmt);
	vsprintf(_buffer,fmt,arg);
	va_end(arg);

	p = _file == stdout ? NONE "\n" : "\n";
	strcat(_buffer,p);
	fwrite(_buffer,strlen(_buffer),1,_file);
}

