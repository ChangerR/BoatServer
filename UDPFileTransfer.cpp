#include "UDPFileTransfer.h"
#include "BitManager.h"
#include "Timer.h"
#include "MD5.h"
#include "NetServer.h"
#include "logger.h"

#define UDPFILEMAGICBEGIN (('B'<<24) + ('S'<<16) + ('T'<<8) + ('B'))
#define UDPFILEMAGICLOOP  (('B'<<24) + ('S'<<16) + ('T'<<8) + ('L'))

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

    virtual ~UDPFilePacket() {
        delete[] _buffer;
    }

	unsigned int getMagic() {
		return *((unsigned int*)(_buffer));
	}

	const char* getTsPacketName() {

		if(getMagic() == UDPFILEMAGICBEGIN)return NULL;
		UDPTSPacket* packet = (UDPTSPacket*)_buffer;

		return packet->filename;
	}

    void debug() {
        Logger::getInstance()->info(0,"[Debug] Magic %c%c%c%c and packet length=%d\n",_buffer[0],_buffer[1],_buffer[2],_buffer[3],_length);
        if(getMagic() == UDPFILEMAGICBEGIN) {
            UDPTSPacketBegin* begin = (UDPTSPacketBegin*)_buffer;
            const unsigned char* out = (unsigned char*)begin->md5;
            Logger::getInstance()->info(0,"[Debug] ---Debug Begin----\n");
            Logger::getInstance()->info(0,"[Debug] recv file package begin\n");
            Logger::getInstance()->info(0,"[Debug] packet count=%d\n",begin->packet_count);
            Logger::getInstance()->info(0,"[Debug] md5=%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X\n",out[0],out[1],out[2],out[3]
        													,out[4],out[5],out[6],out[7]
        													,out[8],out[9],out[10],out[11]
        												    ,out[12],out[13],out[14],out[15]);
            Logger::getInstance()->info(0,"[Debug] recv filename=%s\n",begin->filename);
            Logger::getInstance()->info(0,"[Debug] ---end---\n");
        }else if(getMagic() == UDPFILEMAGICLOOP) {
            UDPTSPacket* packet = (UDPTSPacket*)_buffer;
            Logger::getInstance()->info(0,"[Debug] ---Debug loop----\n");
            Logger::getInstance()->info(0,"[Debug] recv file package loop\n");
            Logger::getInstance()->info(0,"[Debug] packetid=%d\n",packet->packetid);
            Logger::getInstance()->info(0,"[Debug] filepos=%d\n",packet->filepos);
            Logger::getInstance()->info(0,"[Debug] packet length=%d\n",packet->pklen);
            Logger::getInstance()->info(0,"[Debug] packet filename=%s\n",packet->filename);
            Logger::getInstance()->info(0,"[Debug] ---end---\n");
        }else{
            Logger::getInstance()->error("[UDPFileTransfer] recv error packet\n");
        }
    }
public:
    char* _buffer;
    int _uid;
    int _length;
};

class FileTransferTask{
public:
    FileTransferTask(int uid,const char* filepath);
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
    std::string _filepath;
};

FileTransferTask::FileTransferTask(int uid,const char* filepath) {
    _uid = uid;
    memset(_filename,0,64);
    memset(_md5,0,16);
    _timeoutCount = 5;
    _all_packet_count = 0;
    _bit = NULL;
    _file = NULL;
    _filepath = filepath;
}

bool FileTransferTask::init(UDPTSPacketBegin* begin) {
    bool ret = false;
    std::string filename = _filepath;
    do{
        strncpy(_filename,begin->filename,63);
        memcpy(_md5,begin->md5,16);
        _all_packet_count = begin->packet_count;
        _bit = new BitManager(_all_packet_count);
        filename += "/";
        filename += _filename;
        _file = fopen(filename.c_str(),"wb");
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
        _file = NULL;
        std::string filename = _filepath + "/" + _filename;
		if(!MD5::md5sum(filename.c_str(),out))return -2;
		for(int index = 0;index < 16; index++) {
			if(out[index] != _md5[index]) {
				Logger::getInstance()->error("[UDPFileTransfer] Md5 verify Error,please check!\n");
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

void FileTransferTask::handlePacket(UDPFilePacket* fpacket) {
	UDPTSPacket* packet = (UDPTSPacket*)(fpacket->_buffer);
	unsigned char* data = (unsigned char*)(fpacket->_buffer) + sizeof(UDPTSPacket);

	if(_file) {
		fseek(_file,packet->filepos,SEEK_SET);
		fwrite(data,packet->pklen,1,_file);
	}else {
        Logger::getInstance()->error("[UDPFileTransfer] Error file do not opened");
    }
	_bit->setBit(packet->packetid);
	_timeout.reset();
}

UDPFileTransfer::~UDPFileTransfer(){
	for(list<UDPFilePacket*>::node* p = _packets.begin();p != _packets.end();p = p->next) {
		delete p->element;
	}
	for(list<FileTransferTask*>::node* p = _tasks.begin();p != _tasks.end(); p = p->next) {
		delete p->element;
	}
}

void UDPFileTransfer::pushPacket(int id,const char* buf,int len) {
	UDPFilePacket* packet = new UDPFilePacket(id,buf,len);
	_packets.push_back(packet);
}

int UDPFileTransfer::processTasks(NetServer* server) {

	UDPFilePacket* packet = NULL;

	if(!_packets.empty()) {
		packet = _packets.begin()->element;
		_packets.erase(_packets.begin());
	}

	if(packet) {
        //packet->debug();

		if(packet->getMagic() == UDPFILEMAGICBEGIN) {

			FileTransferTask* task = new FileTransferTask(packet->_uid,_filepath.c_str());

			if(task->init((UDPTSPacketBegin*)(packet->_buffer)) == true) {
				_tasks.push_back(task);
			}else
				delete task;

		}else if(packet->getMagic() == UDPFILEMAGICLOOP) {
			for(list<FileTransferTask*>::node* p = _tasks.begin();p != _tasks.end();p = p->next) {
				FileTransferTask* task = p->element;
				if(task->_uid == packet->_uid && !strcmp(task->_filename,packet->getTsPacketName())) {
					task->handlePacket(packet);
					break;
				}
			}
		}else {
			Logger::getInstance()->error("[UDPFileTransfer] Recv error file packet.");
		}

		delete packet;
	}

	for(list<FileTransferTask*>::node* p = _tasks.begin();p != _tasks.end();) {
		FileTransferTask* task = p->element;
		list<FileTransferTask*>::node* next = p->next;
		int iret = task->processTask(server);
		if(iret != 0) {
			if(iret == 1&&_callback != NULL)
				_callback(task->_filename,_user);

			Logger::getInstance()->info(5,"[UDPFileTransfer] Remove task transfer filename=%s ret=%d",task->_filename,iret);
			delete task;
			_tasks.erase(p);
		}
		p = next;
	}

	return _tasks.getSize();
}
