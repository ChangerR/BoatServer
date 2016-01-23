#include "UDPFileTransfer.h"
#include "BitManager.h"
#include "Timer.h"

#define UDPFILEMAGICBEGIN ('B'<<24)|('T'<<16)|('S'<<8)|('B')
#define UDPFILEMAGICLOOP  ('B'<<24)|('T'<<16)|('S'<<8)|('L')

#pragma pack(push,packing)
#pragma pack(1)
struct UDPTSPacketBegin{
    unsigned int magic;
    int packet_count;
    unsigned char md5[16];
    char filename[64];
};

struct UDPTSPacket {
    unsigned int magic;
    int packetid;
    int filepos;
    int pklen;
    char filename[64];
};
#pragma pack(pop,packing)

class FileTransferTask{
public:
    FileTransferTask(int uid);
    virtual ~FileTransferTask();

    bool init(UDPTSPacketBegin* begin);

    bool processTasks();

    void handlePacket(UDPFilePacket* packet);

    bool verifyName(const char* name) {
        return !strcmp(_filename,name);
    }

private:
    BitManager* _bit;
    int _all_packet_count;
    FILE* _file;
    char _filename[64];
    unsigned char _md5[16];
    Timer _timeout;
    int _timeoutCount;
    int _uid;
};

FileTransferTask::FileTransferTask(int uid) {
    _uid = uid;
    memset(_filename,0,64);
    memset(_md5,0,16);
    _timeoutCount = 5;
    _all_packet_count = 0;
    _bit = NULL;
    _file = NULL;
}

bool init(UDPTSPacketBegin* begin) {
    ret =false;
    do{
        strncpy(_filename,beign->filename,63);
        memcpy(_md5,beign->md5,16);
        _all_packet_count = beign->packet_count;
        _bit = new BitManager(_all_packet_count);

        _file = fopen(_filename,"wb");
        if(_file != NULL)
            ret = true;
    }while(0);

    return ret;
}

FileTransferTask::~FileTransferTask() {
    if(_file != NULL) {
        fclose(_file);
    }
    if(_bit != NULL) {
        delete _bit;
    }
}

int FileTransferTask::processTasks(NetServer* server) {

    if(_bit->getFreeBit() == 0 ) {
        fclose(_file);
    }else if(_timeout.elapsed(5000)) {
        char buf[256] = {0};
        if(_timeoutCount < 0)return -1;
        sprintf(buf,"f:::{\"filename\":\"%s\",\"packetid\":%d}",_filename,_bit->getFreeBit());
        server->sendMessageToClient(_uid,buf,strlen(buf));
        _timeoutCount--;
    }

    return 0;
}

void FileTransferTask::handlePacket(UDPFilePacket* packet) {

}

class UDPFilePacket{
public:
    UDPFilePacket(int uid,const char* buf,int len) {
        _buffer = new char[len];
        memcpy(_buffer,buf,len);
        _length = len;
        _uid = uid;
    }

    virtual ~UDPFilePacket() {
        delete[] _buffer;
    }
public:
    char* _buffer;
    int _uid;
    int _length;
};
