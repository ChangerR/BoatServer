#include "NetServer.h"

#define NETSERVER_BUFFER_LEN 4094

NetServer::NetServer(int port,Pilot* pilot) {
    _port = port;
    _pilot = pilot;
    _serverSocket = -1;
    _running = false;
    _buffer = new char[NETSERVER_BUFFER_LEN];
}

NetServer::~NetServer() {
    delete[] _buffer;
}

bool NetServer::init() {
    bool ret = false;
    do {
        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(_port);
        _addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if((_serverSocket = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
            printf("Init Socker Error,please check\n");
            break;
        }

        if(bind(_serverSocket,(struct sockaddr*)&_addr,sizeof(_addr)) < 0) {
            printf("Bind socker Error,please check\n");
            break;
        }

        _running = true;
        ret = true;
    }while(0);
    return ret;
}

void NetServer::handleServerMessage() {
    int recv_len = 0;
    struct sockaddr_in clientaddr;
    fd_set fds;
    struct timeval tv;
    if(!running)return;

}
