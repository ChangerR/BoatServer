#include "NetServer.h"
#include "Pilot.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "json/writer.h"
#include "json/stringbuffer.h"
#include "json/document.h"
#include <dirent.h>
#include "util.h"
#include "logger.h"

#define NETSERVER_BUFFER_LEN 4094

int NetServer::Client::id_count = 1000;

NetServer::Client::Client(struct sockaddr_in& in) {
    _uid = id_count++;
    _time = 60;
    _clientaddr = in;
}

NetServer::NetServer(int port,Pilot* pilot,const char* filepath) {
    _port = port;
    _pilot = pilot;
    _serverSocket = -1;
    _running = false;
    _buffer = new char[NETSERVER_BUFFER_LEN];
	_fileTransfer = new UDPFileTransfer(filepath);
    _filepath = filepath;
}

NetServer::~NetServer() {
	if(_buffer)
		delete[] _buffer;
	if(_fileTransfer)
		delete _fileTransfer;
}

bool NetServer::init() {
    bool ret = false;
    do {
        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(_port);
        _addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if((_serverSocket = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
            Logger::getInstance()->error("[NetServer] Init Socker Error,please check");
            break;
        }

        if(bind(_serverSocket,(struct sockaddr*)&_addr,sizeof(_addr)) < 0) {
            Logger::getInstance()->error("[NetServer] Bind socker Error,please check");
            break;
        }

		_fileTransfer->setFileRecvCallback(NetServer::NetFileRecv,this);

        listControlFiles();

        _running = true;
        ret = true;
        _timer.reset();
    }while(0);
    return ret;
}

void NetServer::closeServer() {
    if(_running) {
        _running = false;
        for(std::map<unsigned long,Client*>::iterator it = _clients.begin(); it != _clients.end();) {
            delete it->second;
            _clients.erase(it++);
        }
        close(_serverSocket);
    }
}

NetServer::Client* NetServer::findClient(struct sockaddr_in& c) {
    std::map<unsigned long,Client*>::iterator it = _clients.find(c.sin_addr.s_addr);

    if(it == _clients.end()) {
        Client* nClient = new Client(c);
        _clients.insert(std::make_pair(c.sin_addr.s_addr,nClient));
        return nClient;
    }

    return it->second;
}

NetServer::Client* NetServer::findClinet(int uid) {
    Client* c = NULL;

    for(std::map<unsigned long,Client*>::iterator it = _clients.begin(); it != _clients.end();++it) {
        if(it->second->_uid == uid) {
            c = it->second;
            break;
        }
    }
    return c;
}

void NetServer::sendMessageToClient(int id,const char* buf,int len) {
    for(std::map<unsigned long,Client*>::iterator it = _clients.begin(); it != _clients.end();++it) {
        if(id == -1||id == it->second->_uid) {
            sendto(_serverSocket,buf,len,0,(struct sockaddr*)&(it->second->_clientaddr),sizeof(struct sockaddr_in));
        }
        if(id == it->second->_uid)break;
    }
}

void NetServer::sendControlFiles(int id) {
    rapidjson::Document d;
    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value _l(rapidjson::kObjectType);
    rapidjson::StringBuffer p;
    rapidjson::Writer<rapidjson::StringBuffer> writer(p);

    d.SetObject();
    d.AddMember("name",rapidjson::Value().SetString("controlfiles"),d.GetAllocator());

    for(list<std::string>::node* p = _controlsFileList.begin(); p != _controlsFileList.end(); p = p->next) {
        args.PushBack(rapidjson::Value().SetString(p->element.c_str()),d.GetAllocator());
    }

    d.AddMember("args",args,d.GetAllocator());

    d.Accept(writer);

    sendMessageToClient(id,p.GetString(),p.Size());
}

void NetServer::listControlFiles() {
    const char* path = _filepath.c_str();
    DIR* p_dir;
    struct dirent* p_dirent;
    _controlsFileList.clear();

    if((p_dir = opendir(path)) == NULL) {
        Logger::getInstance()->error("Cannot opendir %s",path);
        return;
    }
    while((p_dirent = readdir(p_dir))) {
        if(!strcmp(findFileExt(p_dirent->d_name),"lua")) {
            _controlsFileList.push_back(std::string(p_dirent->d_name));
        }
    }
    closedir(p_dir);
    return;
}

bool NetServer::findControlFileInList(const char* filename) {
    bool ret = false;
    for(list<std::string>::node* p = _controlsFileList.begin(); p != _controlsFileList.end(); p = p->next) {
        if(!strcmp(filename,p->element.c_str())){
            ret = true;
            break;
        }
    }
    return ret;
}

bool NetServer::handleServerMessage() {
    int recv_len = 0;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    fd_set fds;
    struct timeval tv = {0,0};
    int id = 0;

    if(!_running)return _running;

    FD_ZERO(&fds);
    FD_SET(_serverSocket,&fds);

    if(select(_serverSocket + 1,&fds,NULL,NULL,&tv) > 0) {

        recv_len = recvfrom(_serverSocket,_buffer,NETSERVER_BUFFER_LEN,MSG_DONTWAIT,
            (struct sockaddr*)&clientaddr,&addr_len);

        if(recv_len > 3 && _buffer[1] == ':' && _buffer[2] == ':' && _buffer[3] == ':') {
            _buffer[recv_len] = 0;
            switch (_buffer[0]) {
                case '2':
                    {
                        Client* c = findClient(clientaddr);
                        Logger::getInstance()->info(1,"[NetServer] Recvive:%s\n",_buffer + 4);
						if(_pilot)
							_pilot->pushControl(c->_uid,_buffer + 4);
                        c->_time = 60;
                    }
                    break;
                case '1':
                    {
                        Client* c = findClient(clientaddr);
                        c->_time = 60;
                        //Logger::getInstance()->error("Reset client uid=%d\n",c->_uid);
                    }
                    break;
                case '3':
                    {
                        Client* c = findClient(clientaddr);
                        c->_time = 60;
                        if(!strcmp("listcontrolfiles",_buffer + 4)) {
                            sendControlFiles(c->_uid);
                        }
                    }
                    break;
                case '4':
                    {
                        if(findControlFileInList(_buffer + 4) == true) {
                            std::string filename = _filepath + "/" + (_buffer + 4);
                            _pilot->setAutoControlScript(filename.c_str());
                            Logger::getInstance()->info(5,"[NetServer] Set autoControl script %s\n",filename.c_str());
                        }
                    }
                    break;
				case 'f':
					{
						Client* c = findClient(clientaddr);
                        c->_time = 60;
						_fileTransfer->pushPacket(c->_uid,_buffer + 4,recv_len - 4);
					}
                    break;
                default:
                    Logger::getInstance()->error("[NetServer] We do not support this cmd:%s\n",_buffer);
                    break;
            }
        } else {
            Logger::getInstance()->error("[NetServer] Recv UNKOWN Message Format\n");
        }
    }

    if(_timer.elapsed(10000)) {
        for(std::map<unsigned long,Client*>::iterator it = _clients.begin(); it != _clients.end();) {
            it->second->_time -= 10;
            if(it->second->_time <= 0) {
                Client* c = it->second;
				if(_pilot != NULL)
					_pilot->cancelControl(c->_uid);
                //Logger::getInstance()->error("Client End Control:UID=%d\n",c->_uid);
                delete c;
                _clients.erase(it++);
            } else {
                ++it;
            }
        }
    }

    if(_pilot != NULL&&_pilot->needBroadcast(&id,_buffer,&recv_len)){
        sendMessageToClient(id,_buffer,recv_len);
    }
	_fileTransfer->processTasks(this);

    return _running;
}

void NetServer::NetFileRecv(const char* filename,void* user) {
	NetServer* server = (NetServer*)user;
    if(!strcmp(findFileExt(filename),"lua")&&server->findControlFileInList(filename) == false)
	   server->_controlsFileList.push_back(std::string(filename));
    Logger::getInstance()->info(5,"[NetServer] Recv File %s\n",filename);
}
