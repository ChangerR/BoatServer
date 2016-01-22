#ifndef _UDPFILETRANSFER_H
#define _UDPFILETRANSFER_H
#include <string>
#include "list.h"
#include "NetServer.h"

class FileTransferTask;
class UDPFilePacket;

class UDPFileTransfer {
public:
    UDPFileTransfer();
    virtual ~UDPFileTransfer();

    void pushPacket(int id,const char* buf,int len);

    int processTasks(NetServer* server);
private:
    std::string _filepath;
    list<FileTransferTask*> _tasks;
    list<UDPFilePacket*> _packets;
};
#endif
