#include "Pilot.h"
#include "json/writer.h"
#include "json/stringbuffer.h"

Pilot::Pilot(Hardware* h,const char* script) {
    _hardware = h;
    _PilotState = PILOT_MANUAL;
    pthread_mutex_init(&_RecvLock,NULL);
    pthread_mutex_init(&_SendLock,NULL);
    _status = new HWStatus::Status;
    _thruster1id = _thruster2id = 0;
    _thruster1 = _thruster2 = 0;
    _motor1id = _motor2id = _motor3id = _motor4id = 0;
    _motor1 = _motor2 = _motor3 = _motor4 = 0;
    _led1id = _led2id = 0;
    _led1 = _led2 = 0;
    _autoController = new AutoController;
}

Pilot::~Pilot() {
    pthread_mutex_lock(&_RecvLock);
    for(list<_Cmd*>::node * p = _RecvList.begin();p != _RecvList.end();p = p->next) {
        delete p->element;
    }
    pthread_mutex_unlock(&_RecvLock);

    pthread_mutex_lock(&_SendLock);
    for(list<_Cmd*>::node * p = _SendList.begin();p != _SendList.end();p = p->next) {
        delete p->element;
    }
    pthread_mutex_unlock(&_SendLock);

    pthread_mutex_destroy(&_RecvLock);
    pthread_mutex_destroy(&_SendLock);
    delete _status;
}

void Pilot::pushControl(int id,const char* cmd) {
    _Cmd* command = new _Cmd;

    command->id = id;
    command->cmd = cmd;

    pthread_mutex_lock(&_RecvLock);
    _RecvList.push_back(command);
    pthread_mutex_unlock(&_RecvLock);
}

void Pilot::cancelControl(int id) {
    _Cmd* command = new _Cmd;

    command->id = id;
    command->cmd = "{\"name\":\"cancelControl\",\"args\":[]}";

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

void Pilot::setAutoControlScript(const char* script) {
	_autoScript = script;
}

void Pilot::handleControl() {

    _Cmd* command = NULL;
    rapidjson::Document* doc = NULL;
    int uid = -1;
    pthread_mutex_lock(&_RecvLock);
    if(!_RecvList.empty()){
        command = _RecvList.begin()->element;
        _RecvList.erase(_RecvList.begin());
    }
    pthread_mutex_unlock(&_RecvLock);

    if(command) {
        bool ret = true;
        uid = command->id;
        doc = new rapidjson::Document;
        do{
            doc->Parse<0>(command->cmd.c_str());
            if(doc->HasParseError()||doc->IsObject() == false||doc->HasMember("name") == false||
                    doc->HasMember("args") == false ) {
                ret = false;
                break;
            }
            if(!strcmp((*doc)["name"].GetString(),"ControlState")) {
                const rapidjson::Value& pArgs = (*doc)["args"];
                if(pArgs.IsArray()&&pArgs[rapidjson::SizeType(0)].IsInt()) {
                    int s = pArgs[rapidjson::SizeType(0)].GetInt();
                    _PilotState = s;
                    resetControl();
                    if(_PilotState == PILOT_AUTOCONTROL&&_autoScript.length() != 0) {
                        _autoController.close();
                        _autoController.init(_autoScript.c_str(),this);
                    }
                }
                ret = false;
            }

        }while(0);

        if(ret == false) {
            delete doc;
            doc = NULL;
        }
        delete command;
    }

    if(_StatusUpdate.elapsed(500)){
        hardware->requestStatus();
    }

    if(_hardware->getStatus()->isAllUpdated() == true) {
        _hardware->getStatus()->copyStatus(_status);
    }

    if(_sendStatusTimer.elapsed(1000)) {
        sendStatus();
    }

    switch (_PilotState) {
        case PILOT_MANUAL:
            if(doc)
                manualControl(doc,uid);
            break;
        case PILOT_HALFMANUAL:
            if(doc)
                halfManualControl(doc,uid);
            break;
        case PILOT_AUTOCONTROL:
            autoControl();
            break;
        default:
            _PilotState = PILOT_MANUAL;
            break;
    }

    if(doc != NULL)
        delete doc;
}

void Pilot::sendStatus() {
    rapidjson::Document d;
    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value _l(rapidjson::kObjectType);
    rapidjson::StringBuffer p;
    rapidjson::Writer<rapidjson::StringBuffer> writer(p);

    d.SetObject();
    d.AddMember("name",rapidjson::Value().SetString("status"),d.GetAllocator());

    _l.AddMember("laser1",rapidjson::Value().SetInt(_status->laser1),d.GetAllocator());
    _l.AddMember("laser2",rapidjson::Value().SetInt(_status->laser2),d.GetAllocator());
    _l.AddMember("laser3",rapidjson::Value().SetInt(_status->laser3),d.GetAllocator());
    _l.AddMember("laser4",rapidjson::Value().SetInt(_status->laser4),d.GetAllocator());
    _l.AddMember("laser5",rapidjson::Value().SetInt(_status->laser5),d.GetAllocator());
    _l.AddMember("roll",rapidjson::Value().SetDouble(_status->roll),d.GetAllocator());
    _l.AddMember("yaw",rapidjson::Value().SetDouble(_status->yaw),d.GetAllocator());
    _l.AddMember("pitch",rapidjson::Value().SetDouble(_status->pitch),d.GetAllocator());
    _l.AddMember("latitude",rapidjson::Value().SetDouble(_status->latitude),d.GetAllocator());
    _l.AddMember("longitude",rapidjson::Value().SetDouble(_status->longitude),d.GetAllocator());
    _l.AddMember("height",rapidjson::Value().SetDouble(_status->height),d.GetAllocator());
    _l.AddMember("speed",rapidjson::Value().SetDouble(_status->speed),d.GetAllocator());
    _l.AddMember("time",rapidjson::Value().SetDouble(_status->time),d.GetAllocator());
    _l.AddMember("updateTime",rapidjson::Value().SetUint64(_status->timegap + _status->timer.timegap()),d.GetAllocator());
	_l.AddMember("controlState",rapidjson::Value().SetInt(_PilotState),d.GetAllocator());
	
    args.PushBack(_l,d.GetAllocator());
    d.AddMember("args",args,d.GetAllocator());

    d.Accept(writer);
    pushBroadcast(-1,p.GetString());
}

void Pilot::manualControl(rapidjson::Document* doc,int uid) {
    const rapidjson::Value& pArgs = (*doc)["args"];
    if(pArgs.IsArray() == false)return;

    if(!strcmp("cancelControl",(*doc)["name"].GetString())) {
        printf("Cancel uid=%d Control\n",uid);
		cancelIdControl(uid);
    } else if(pArgs[rapidjson::SizeType(0)].IsInt()) {
        int s = pArgs[rapidjson::SizeType(0)].GetInt();
        if(!strcmp("thro",(*doc)["name"].GetString())) {
            printf("thro control power=%d\n",s);
			sendThruster(uid,1,s);
			sendThruster(uid,2,s);
        } else if(!strcmp("yaw",(*doc)["name"].GetString())) {
            printf("yaw control power=%d\n",s);
			sendThruster(uid,1,s);
			sendThruster(uid,2,-s);
        }
    }
}

void Pilot::halfManualControl(rapidjson::Document* doc,int uid) {
    pushBroadcast(uid,"Sorry,we do not support this Control");
}

void Pilot::autoControl() {
    if(_status->isUpdated == true) {
        _autoController->runController();
        _status->isUpdated == false;
    }
}

void Pilot::resetControl() {
    sendThruster(0,1,0);
    sendThruster(0,2,0);
    sendThruster(0,3,0);
    sendThruster(0,4,0);
    sendThruster(0,5,0);
    sendThruster(0,6,0);
    sendLED(0,1,0);
    sendLED(0,2,0);
}

void Pilot::pushBroadcast(int id,const char* cmd) {
    _Cmd* command = new _Cmd;

    command->id = id;
    command->cmd = cmd;

    pthread_mutex_lock(&_SendLock);
    _SendList.push_back(command);
    pthread_mutex_unlock(&_SendLock);
}

void Pilot::cancelIdControl(int id) {

	if(id == _thruster1id)
		sendThruster(0,1,0);
	if(id == _thruster2id)
		sendThruster(0,2,0);
	if(id == _motor1id)
		sendThruster(0,3,0);
	if(id == _motor2id)
		sendThruster(0,4,0);
	if(id == _motor3id)
		sendThruster(0,5,0);
	if(id == _motor4id)
		sendThruster(0,6,0);
}