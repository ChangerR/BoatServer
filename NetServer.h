#ifndef _BOAT_NETSERVER_
#define _BOAT_NETSERVER_
#include "Timer.h"
#include <netinet/in.h>
#include <map>
#include "UDPFileTransfer.h"

class Pilot;

class NetServer{
public:
    NetServer(int port,Pilot* pilot,const char* filepath);
    virtual ~NetServer();

    bool init();
    bool handleServerMessage();

    void closeServer();

    void sendMessageToClient(int id,const char* buf,int len);

    class Client {
    public:
        Client(struct sockaddr_in& in);
        int _uid;
        int _time;
        sockaddr_in _clientaddr;
        static int id_count;
    };

	static void NetFileRecv(const char* filename,void* user);
private:
    Client* findClient(struct sockaddr_in& c);
    Client* findClinet(int uid);

    int _port;
    Pilot* _pilot;
    int _serverSocket;
    sockaddr_in _addr;
    bool _running;
    char* _buffer;
    std::map<unsigned long,Client*> _clients;
    Timer _timer;
	UDPFileTransfer* _fileTransfer;
};
#endif
