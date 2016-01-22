#ifndef __BOATSERVER_AUTOCONTROL_
#define __BOATSERVER_AUTOCONTROL_
#include "lua.hpp"
#include "Pilot.h"

class AutoController {
public:
    AutoController():_L(NULL),_pilot(NULL),_isRunning(false){};
    virtual ~AutoController(){close()};

    bool init(const char* script,Pilot* pilot);

    void close();

    int runController();

    static int getStatus(lua_State* L);

    static int log(lua_State* L);

    static int setThruster(lua_State* L);

    static int setLED(lua_State* L);

private:
    lua_State* _L;
    Pilot* _pilot;
    bool _isRunning;
    static struct luaL_reg _controller[];
};
#endif
