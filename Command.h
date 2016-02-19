#ifndef __SL_ARDUINO_CMD_H
#define __SL_ARDUINO_CMD_H
#include "slconfig.h"
#include "Ref.h"
#include <map>
#include <string>

#define MAX_CMD_ARGUMENT_LEN 256
#define MAX_CMD_ARGS 16
#define MAX_INOCMD 128
typedef int (*ino_execute)(int,char (*)[MAX_CMD_ARGUMENT_LEN],void*);

struct ino_command {
	ino_execute exec;
	void* user;
};

class CmdEvent: public Ref{
public:
	CmdEvent();
	virtual ~CmdEvent();

	void addEvent(const char* ev_name,ino_execute exec,void* user);

	int fireEvent(const char* ev,int,char (*)[MAX_CMD_ARGUMENT_LEN]);

	void removeEvent(const char* ev);
private:
	std::map<std::string,ino_command> _events;
};

class Command {
public:
	Command(CmdEvent* cmdevent);
	virtual ~Command();

	bool parse(const char c);

	void parseWithFire(const char*);

	void reset();

public:
	char argv[MAX_CMD_ARGS][MAX_CMD_ARGUMENT_LEN];
	int args;
	ino_command inocmd[MAX_INOCMD];
	int cmd_count;
	char cmd[MAX_CMD_ARGUMENT_LEN];
	int parseState;
	int byteOffset;
	CmdEvent* _event;
};

#endif
