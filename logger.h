#ifndef __BOAT_LOGGER_H
#define __BOAT_LOGGER_H
#include <stdio.h>

class Logger {
public:
	Logger(const char* filename);
	virtual ~Logger();

	bool init();

	static Logger* getInstance();

	static void destoryInstance();
	
	static void setInstaceFile(const char* filename);

	void info(int level,const char* fmt,...);

	void warning(const char* fmt,...);

	void error(const char* fmt,...);

	void setLogLevel(int l);

private:
	static char _instanceFileName[];
	static Logger* _instance;
	char _filename[256];
	FILE* _file;
	int _level;
	char* _buffer;
};
#endif
