#ifndef _BOAT_NETSERVER_
#define _BOAT_NETSERVER_
#include <sys/types.h>
#include <sys/socket.h>
class Pilot;

class NetServer{
public:
    NetServer(int port,Pilot* pilot);
    virtual ~NetServer();

    bool init();
    void handleServerMessage();

    void close();
public:
    int _port;
    Pilot* _pilot;
    int _serverSocket;
    sockaddr_in _addr;
    bool _running;
    char* _buffer;
};
#endif
