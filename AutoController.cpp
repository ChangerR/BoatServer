#include "AutoController.h"
#include <stdio.h>
#include "json/writer.h"
#include "json/stringbuffer.h"
#include "Pilot.h"
#include "logger.h"

struct luaL_reg AutoController::_controller[] = {
    {"status",AutoController::getStatus},
    {"thruster",AutoController::setThruster},
    {"led",AutoController::setLED},
    {"log",AutoController::log},
    {"timerReset",AutoController::timerReset},
    {"elapsed",AutoController::timerElapsed},
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

bool AutoController::init(const char* script,Pilot* pilot) {
    bool ret = false;

    do {
        if(script == NULL||pilot == NULL)
            break;

        _pilot = pilot;
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

        lua_pushlightuserdata(_L,_pilot);
        lua_setglobal(_L,"_hidden_pilot_");

        _isRunning = true;
        ret = true;

    }while(0);

    return ret;
}

void AutoController::close() {

    if(_isRunning) {
        lua_close(_L);
        _L = NULL;
        _isRunning = false;
    }
}

int AutoController::runController() {

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

int AutoController::getStatus(lua_State* L) {

    lua_getglobal(L,"_hidden_pilot_");
    Pilot* pilot = (Pilot*)lua_touserdata(L,-1);

    lua_pop(L,1);
    lua_newtable(L);
    if(pilot == NULL) return 1;

    HWStatus::Status* status = pilot->_status;

    l_setfield(L,"laser1",status->laser1);
    l_setfield(L,"laser2",status->laser2);
    l_setfield(L,"laser3",status->laser3);
    l_setfield(L,"laser4",status->laser4);
    l_setfield(L,"laser5",status->laser5);
    l_setfield(L,"roll",status->roll);
    l_setfield(L,"pitch",status->pitch);
    l_setfield(L,"yaw",status->yaw);
    l_setfield(L,"longitude",status->longitude);
    l_setfield(L,"latitude",status->latitude);
    l_setfield(L,"speed",status->speed);
    l_setfield(L,"time",status->time);
    l_setfield(L,"timegap",int(status->timer.timegap()));
    l_setfield(L,"updated",status->isUpdated ? 1:0);

    return 1;
}

int AutoController::log(lua_State* L) {
    lua_getglobal(L,"_hidden_pilot_");
    Pilot* pilot = (Pilot*)lua_touserdata(L,-1);
    rapidjson::Document d;
    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::StringBuffer p;
    rapidjson::Writer<rapidjson::StringBuffer> writer(p);
    size_t slen = 0;

    lua_pop(L,1);
    if(pilot == NULL) return 0;
    const char* logs = luaL_checklstring(L,1,&slen);

    d.SetObject();
    d.AddMember("name",rapidjson::Value().SetString("log"),d.GetAllocator());
    args.PushBack(rapidjson::Value().SetString(logs),d.GetAllocator());
    d.AddMember("args",args,d.GetAllocator());

    d.Accept(writer);
    pilot->pushBroadcast(-1,p.GetString());

    return 0;
}

int AutoController::setThruster(lua_State* L) {
    lua_getglobal(L,"_hidden_pilot_");
    Pilot* pilot = (Pilot*)lua_touserdata(L,-1);

    lua_pop(L,1);
    if(pilot == NULL) return 0;
    int device = luaL_checkint(L,1);
    int power = luaL_checkint(L,2);

    pilot->sendThruster(1,device,power);

    return 0;
}

int AutoController::setLED(lua_State* L) {
    lua_getglobal(L,"_hidden_pilot_");
    Pilot* pilot = (Pilot*)lua_touserdata(L,-1);

    lua_pop(L,1);
    if(pilot == NULL) return 0;
    int device = luaL_checkint(L,1);
    int power = luaL_checkint(L,2);

    pilot->sendLED(1,device,power);

    return 0;
}

int AutoController::timerReset(lua_State* L) {
    lua_getglobal(L,"_hidden_pilot_");
    Pilot* pilot = (Pilot*)lua_touserdata(L,-1);

    lua_pop(L,1);
    if(pilot == NULL) return 0;
    AutoController* control = pilot->_autoController;

    if(control != NULL) {
        control->_timer.reset();
    }

    return 0;
}

int AutoController::timerElapsed(lua_State* L) {
    lua_getglobal(L,"_hidden_pilot_");
    Pilot* pilot = (Pilot*)lua_touserdata(L,-1);

    lua_pop(L,1);
    if(pilot == NULL) return 0;
    AutoController* control = pilot->_autoController;
    int timeout = luaL_checkint(L,1);
    int ret = 0;
    if(control != NULL) {
        ret = control->_timer.elapsed(timeout);
    }
    lua_pushboolean(L,ret);

    return 1;
}
