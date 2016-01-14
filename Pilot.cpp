#include "Pilot.h"
#include "json/document.h"

Pilot::Pilot(Hardware* h) {
    _hardware = h;
    pthread_mutex_init(&_RecvLock,NULL);
    pthread_mutex_init(&_SendLock,NULL);
}

Pilot::~Pilot() {
    pthread_mutex_destroy(&_RecvLock);
    pthread_mutex_destroy(&_SendLock);
}

void Pilot::pushControl(int id,const char* cmd) {
    _Cmd* command = new _Cmd;

    command.id = id;
    command.cmd = cmd;

    pthread_mutex_lock(&_RecvLock);
    _RecvList.push_back(command);
    pthread_mutex_unlock(&_RecvLock);
}

void Pilot::cancelControl(int id) {
    _Cmd* command = new _Cmd;

    command.id = id;
    command.cmd = "{\"name\":\"cancelControl\",\"args\":[]}";

    pthread_mutex_lock(&_RecvLock);
    _RecvList.push_back(command);
    pthread_mutex_unlock(&_RecvLock);
}

bool Pilot::needBroadcast(int* id,char* buf,int* len) {
    _Cmd* command = NULL;

    pthread_mutex_lock(&_SendLock);
    if(!_SendList.empty()){
        command = _SendList.begin()->element;
        _SendList.erase(_SendList.begin());
    }
    pthread_mutex_unlock(&_SendLock);

    if(command) {
        *id = command->id;
        *len = command->cmd.size();
        strcpy(buf,command->cmd.c_str());
        delete command;
        return true;
    }else
        return false;
}

void Pilot::handleControl() {
    if(_hardware == NULL)return;
    _Cmd* command = NULL;
    rapidjson::Document* doc = NULL;

    pthread_mutex_lock(&_RecvLock);
    if(!_RecvList.empty()){
        command = _RecvList.begin()->element;
        _RecvList.erase(_RecvList.begin());
    }
    pthread_mutex_unlock(&_RecvLock);

    if(command) {
        bool ret = true;
        do{
            doc->Parse<0>(command->cmd.c_str());
            if(doc->HasParseError()||doc->IsObject() == false||doc->HasMember("name") == false||
                    doc->HasMember("args") == false) {
                ret = false;
                break;
            }
            if(!strcmp((*doc)["name"].GetString(),"state")) {
                
            }
        }while(0);
    }
}
