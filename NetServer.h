#ifndef _BOAT_NETSERVER_
#define _BOAT_NETSERVER_
#include "Timer.h"
#include <netinet/in.h>
#include <map>
class Pilot;

class NetServer{
public:
    NetServer(int port,Pilot* pilot);
    virtual ~NetServer();

    bool init();
    bool handleServerMessage();

    void closeServer();

    class Client {
    public:
        Client(struct sockaddr_in& in);
        int _uid;
        int _time;
        sockaddr_in _clientaddr;
        static int id_count;
    };
private:
    Client* findClient(struct sockaddr_in& c);
    int _port;
    Pilot* _pilot;
    int _serverSocket;
    sockaddr_in _addr;
    bool _running;
    char* _buffer;
    std::map<unsigned long,Client*> _clients;
    Timer _timer;
};
#endif
