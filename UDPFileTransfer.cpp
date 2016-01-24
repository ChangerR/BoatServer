#include "UDPFileTransfer.h"
#include "BitManager.h"
#include "Timer.h"
#include "MD5.h"
#include "NetServer.h"

#define UDPFILEMAGICBEGIN ('B'<<24)|('S'<<16)|('T'<<8)|('B')
#define UDPFILEMAGICLOOP  ('B'<<24)|('S'<<16)|('T'<<8)|('L')

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

class UDPFilePacket{
public:
    UDPFilePacket(int uid,const char* buf,int len) {
        _buffer = new char[len];
        memcpy(_buffer,buf,len);
        _length = len;
        _uid = uid;
    }
	
	unsigned int getMagic() {
		return *((unsigned int*)(_buffer));
	}
	
	const char* getTsPacketName() {
			
		if(getMagic() != UDPFILEMAGICBEGIN)return NULL;
		UDPTSPacket* packet = (UDPFilePacket*)_buffer;
		
		return packet->filename;
	}

    virtual ~UDPFilePacket() {
        delete[] _buffer;
    }
public:
    char* _buffer;
    int _uid;
    int _length;
};

class FileTransferTask{
public:
    FileTransferTask(int uid);
    virtual ~FileTransferTask();

    bool init(UDPTSPacketBegin* begin);

    int processTask(NetServer* server);

    void handlePacket(UDPFilePacket* packet);

private:
    BitManager* _bit;
    int _all_packet_count;
    FILE* _file;
    char _filename[64];
    unsigned char _md5[16];
    Timer _timeout;
    int _timeoutCount;
    int _uid;
	friend class UDPFileTransfer;
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

int FileTransferTask::processTask(NetServer* server) {
	char buf[256] = {0};
    if(_bit->getFreeBit() == 0 ) {
		unsigned char out[16] = {0};
        fclose(_file);
		
		if(!MD5::md5sum(_filename,out))return -2;
		for(int index = 0;index < 16; index++) {
			if(out[index] != _md5[index]) {
				printf("Md5 verify Error,please check!\n");
				return -2;
			}
		}
		sprintf(buf,"{\"name\":\"filetrans\",\"args\":[{\"type\":0,\"filename\":\"%s\"}]}",_filename);
		server->sendMessageToClient(_uid,buf,strlen(buf));
		
		return 1;
    }else if(_timeout.elapsed(5000)) {
        
        if(_timeoutCount < 0)return -1;
        sprintf(buf,"{\"name\":\"filetrans\",\"args\":[{\"type\":1,\"filename\":\"%s\",\"packet\":%d}]}",
						_filename,_bit->getFreeBit());
        server->sendMessageToClient(_uid,buf,strlen(buf));
        _timeoutCount--;
    }

    return 0;
}

void FileTransferTask::handlePacket(UDPFilePacket* packet) {
	UDPTSPacket* packet = (UDPTSPacket*)packet->_buffer;
	char* data = (unsigned char*)packet->_buffer + sizeof(UDPTSPacket);
	
	if(_file) {
		fseek(_file,packet->filepos,SEEK_SET);
		fwrite(data,packet->pklen,1,_file);
	}
	_bit->setBit(packet->packetid);
	_timeout.reset();
}

UDPFileTransfer::~UDPFileTransfer(){
	for(list<UDPFilePacket*>::node* p = _packets.begin();p != _packets.end();p = p->next) {
		delete p->element;
	}
	for(list<FileTransferTask*>node* p = _tasks.begin();p != _tasks.end(); p = p->next) {
		delete p->element;
	}
}

void pushPacket(int id,const char* buf,int len) {
	UDPFilePacket* packet = new UDPFilePacket(id,buf,len);
	_packets.push_back(packet);
}

int processTasks(NetServer* server) {
	
	UDPFilePacket* packet = NULL;
	
	if(!_packets.empty()) {
		packet = _packets.begin()->element;
		_packets.erase(_packets.begin());
	}
	
	if(packet) {
		if(packet->getMagic() == UDPFILEMAGICBEGIN) {
			FileTransferTask* task = new FileTransferTask(packet->_uid);
			
			if(task.init((UDPTSPacketBegin*)packet->_buffer) == true) {
				_tasks.push_back(task);
			}else
				delete task;
			
		}else if(packet->getMagic() == UDPFILEMAGICLOOP) {
			for(list<FileTransferTask*>::node* p = _tasks.begin();p != _tasks.end();p = p->next) {
				FileTransferTask* task = p->element;
				if(task->_uid == packet->_uid&&!strcmp(task->_filename,packet->getTsPacketName()) == 0) {
					task->handlePacket((UDPTSPacket*)packet->_buffer);
					break;
				}
			}
		}else {
			printf("Error:recv error file packet\n");
		}
		delete packet;
	}
	
	for(list<FileTransferTask*>::node* p = _tasks.begin();p != _tasks.end();) {
		FileTransferTask* task = p->element;
		list<FileTransferTask*>::node* next = p->next;
		int iret = task->processTask(server);
		if(iret != 0) {
			if(iret == 1&&_calllback != NULL)
				_calllback(task->_filename,_user);
				
			printf("Remove task transfer filename=%s\n",task->_filename);
			delete task;
			_tasks.erase(p);
		}
		p = next;
	}
	
	return _tasks.getSize();
}