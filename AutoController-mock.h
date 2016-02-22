#ifndef __BOATSERVER_ATUOCONTROL_MOCK_
#define __BOATSERVER_ATUOCONTROL_MOCK_
#include "lua.hpp"

class AutoControllerMock {
public:
	AutoControllerMock():_L(NULL),_isRunning(false){};
	virtual ~AutoControllerMock(){};

	bool init(const char* script);

    void close();

    int runController();

    static int getStatus(lua_State* L);

    static int log(lua_State* L);

    static int setThruster(lua_State* L);

    static int setLED(lua_State* L);

    static int timerReset(lua_State* L);

    static int timerElapsed(lua_State* L);
private:
    lua_State* _L;
    bool _isRunning;
    static struct luaL_reg _controller[];
};
#endif
