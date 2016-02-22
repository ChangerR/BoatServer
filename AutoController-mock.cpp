#include "AutoController-mock.h"
#include "logger.h"

struct luaL_reg AutoControllerMock::_controller[] = {
    {"status",AutoControllerMock::getStatus},
    {"thruster",AutoControllerMock::setThruster},
    {"led",AutoControllerMock::setLED},
    {"log",AutoControllerMock::log},
    {"timerReset",AutoControllerMock::timerReset},
    {"elapsed",AutoControllerMock::timerElapsed},
    {NULL,NULL},
};

inline void l_setfield(lua_State* L,const char* index,int value) {
    lua_pushstring(L,index);
    lua_pushinteger(L,value);
    lua_settable(L,-3);
}

inline void l_setfield(lua_State* L,const char* index,float f) {
    lua_pushstring(L,index);
    lua_pushnumber(L,f);
    lua_settable(L,-3);
}

bool AutoControllerMock::init(const char* script) {
    bool ret = false;

    do {
        if(script == NULL)
            break;

        _L = lua_open();

        if(_L == NULL)
            break;

        luaL_openlibs(_L);
        luaL_openlib(_L,"controller",_controller,0);

        if(luaL_dofile(_L,script)) {
			Logger::getInstance()->error("Compile Script Error:%s\n",lua_tostring(_L,-1));
            lua_pop(_L,1);
            lua_close(_L);
            _L = NULL;
            break;
        }

        _isRunning = true;
        ret = true;

    }while(0);

    return ret;
}

void AutoControllerMock::close() {

    if(_isRunning) {
        lua_close(_L);
        _L = NULL;
        _isRunning = false;
    }
}

int AutoControllerMock::runController() {

    if(_isRunning == false)return -1;

    lua_getglobal(_L,"loop");
    if(lua_pcall(_L,0,1,0)) {
		Logger::getInstance()->error("Run Script Error=%s\n",lua_tostring(_L,-1));
        lua_pop(_L,1);
        return -1;
    }
    int ret = lua_tointeger(_L,-1);
    lua_pop(_L,1);

    return ret;
}

int AutoControllerMock::getStatus(lua_State* L) {

    lua_newtable(L);
	
    l_setfield(L,"laser1",1000);
    l_setfield(L,"laser2",1000);
    l_setfield(L,"laser3",1000);
    l_setfield(L,"laser4",1000);
    l_setfield(L,"laser5",1000);
    l_setfield(L,"roll",1000);
    l_setfield(L,"pitch",1000);
    l_setfield(L,"yaw",1000);
    l_setfield(L,"longitude",1000);
    l_setfield(L,"latitude",1000);
    l_setfield(L,"speed",1000);
    l_setfield(L,"time",1000);
    l_setfield(L,"timegap",1000);
    l_setfield(L,"updated",1);

	Logger::getInstance()->info(5,"Call AutoController::getStatus()");

    return 1;
}

int AutoControllerMock::log(lua_State* L) {
    size_t slen = 0;

	const char* logs = luaL_checklstring(L,1,&slen);

	Logger::getInstance()->info(5,"script log:%s",logs);

    return 0;
}

int AutoControllerMock::setThruster(lua_State* L) {
    int device = luaL_checkint(L,1);
    int power = luaL_checkint(L,2);

	Logger::getInstance()->info(5,"set thruster device=%d power=%d",device,power);

    return 0;
}

int AutoControllerMock::setLED(lua_State* L) {
    int device = luaL_checkint(L,1);
    int power = luaL_checkint(L,2);

    Logger::getInstance()->info(5,"set led device=%d power=%d",device,power);

    return 0;
}

int AutoControllerMock::timerReset(lua_State* L) {
   	
	Logger::getInstance()->info(5,"call timer reset");

    return 0;
}

int AutoControllerMock::timerElapsed(lua_State* L) {
    int timeout = luaL_checkint(L,1);

	Logger::getInstance()->info(5,"call timer elapsed=%d",timeout);

    lua_pushboolean(L,true);

    return 1;
}

