#include "UDPFileTransfer.h"
#include "BitManager.h"

#define UDPFILEMAGICBEGIN ('B'<<24)|('T'<<16)|('S'<<8)|('B')
#define UDPFILEMAGICLOOP  ('B'<<24)|('T'<<16)|('S'<<8)|('L')

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

class FileTransferTask{
public:
    FileTransferTask();
    virtual ~FileTransferTask();
private:
    BitManager _bit;
};

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
