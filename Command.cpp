#include "Command.h"
#include <string.h>
#include <stdio.h>

CmdEvent::CmdEvent(){};
CmdEvent::~CmdEvent(){};

void CmdEvent::addEvent(const char* ev_name,ino_execute exec,void* user) {
	ino_command cmd = {0};

	cmd.exec = exec;
	cmd.user = user;
	_events.insert(std::pair<std::string,ino_command>(ev_name,cmd));
}

int CmdEvent::fireEvent(const char* ev,int args,char (*argv)[MAX_CMD_ARGUMENT_LEN]) {

	if(_events.find(ev) != _events.end()) {
		ino_command cmd = _events.at(ev);
		return cmd.exec(args,argv,cmd.user);
	}else
		return -1;
}

void CmdEvent::removeEvent(const char* ev) {

	if(_events.find(ev) != _events.end()) {
		_events.erase(ev);
	}
}

#define PARSE_COMMAND 0
#define PARSE_ARGS    1
#define PARSE_STRING  2
#define PARSE_ERROR   3
#define PARSE_END     4

Command::Command(CmdEvent* cmdevent) {
	args = 0;
	cmd_count = 0;
	byteOffset = 0;
	parseState = PARSE_COMMAND;
	_event =  cmdevent;
	_event->retain();
}

Command::~Command() {
	_event->release();
}

bool Command::parse(const char c) {

	if(byteOffset >= MAX_CMD_ARGUMENT_LEN) {
		byteOffset = 0;
		parseState = PARSE_ERROR;
	} if(PARSE_COMMAND == parseState) {

		if(c != '(')
			cmd[byteOffset++] = c;
		else {
			cmd[byteOffset++] = 0;
			byteOffset = 0;
			args = 0;
			parseState = PARSE_ARGS;
		}

	}else if(PARSE_ARGS == parseState) {

		if(c == ',') {
			argv[args][byteOffset++] = 0;
			byteOffset = 0;
			args++;
			if(args > MAX_CMD_ARGS) {
				parseState = PARSE_ERROR;
			}
		}else if(c == ')') {
			argv[args][byteOffset++] = 0;
			args++;
			if( argv[0][0] == 0)
				args = 0;
			parseState = PARSE_END;
			return true;
		}else if(c == '\"') {
			parseState = PARSE_STRING;
		}else if(c != ' '&&c != '\t') {
			argv[args][byteOffset++] = c;
		}


	}else if(PARSE_STRING == parseState) {

		if( c != '\"' )
			argv[args][byteOffset++] = c;
		else
			parseState = PARSE_ARGS;

	}else if(PARSE_ERROR == parseState) {

		if( c == ')') {
			args = 0;
			*cmd = 0;
			parseState = PARSE_END;
			return false;
		}
	}else {
		if(c != ' '&&c != '\t'&&c != '\r'&&c != '\n') {
			parseState = PARSE_COMMAND;
			byteOffset = 0;
			cmd[byteOffset++] = c;
		}
	}

	return false;
}

void Command::parseWithFire(const char* p) {

	while(*p) {
		if(parse(*p) == true) {
			_event->fireEvent(cmd,args,argv);
		}
		p++;
	}
}
