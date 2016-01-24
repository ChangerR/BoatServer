#ifndef _UDPFILETRANSFER_H
#define _UDPFILETRANSFER_H
#include <string>
#include "list.h"

class FileTransferTask;
class UDPFilePacket;
class NetServer;

typedef void (*FileRecvCallback)(const char* str,void* user);

class UDPFileTransfer {
public:
    UDPFileTransfer(const char* filepath):_filepath(filepath),_callback(NULL),_user(NULL){};
    virtual ~UDPFileTransfer();

    void pushPacket(int id,const char* buf,int len);

    int processTasks(NetServer* server);
	
	void setFileRecvCallback(FileRecvCallback callback,void *data) {
		_callback = callback;
		_user = data;
	}
private:
    std::string _filepath;
    list<FileTransferTask*> _tasks;
    list<UDPFilePacket*> _packets;
	FileRecvCallback _callback;
	void* _user;
};
#endif
