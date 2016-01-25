#ifndef _BOAT_BITMANAGER_
#define _BOAT_BITMANAGER_
#include <memory.h>
#include <stdio.h>
class BitManager {
public:
    BitManager(int count) {
        int len = (count + 7) >> 3;
        _buffer = new unsigned char[len];
        memset(_buffer,0,len);
        _count = count;
    }

    virtual ~BitManager() {
        delete[] _buffer;
    }

    void setBit(int id) {

        if(id > _count ||id <= 0)return;
        id--;
        int pos = id >> 3;
        int bit = id & 0x7;

        _buffer[pos] |= (1 << bit);
    }

    int getFreeBit() {
        int len = (_count + 7) >> 3;
        int pos = 0;
        for(int i = 0; i < len;i++) {
            if(_buffer[i] != 0xff) {
                if((_buffer[i] & 0x1) == 0) {
                    pos = i * 8 + 0x1;
                    break;
                } else if((_buffer[i] & 0x2) == 0) {
                    pos = i * 8 + 0x2;
                    break;
                } else if((_buffer[i] & 0x4) == 0) {
                    pos = i * 8 + 0x3;
                    break;
                } else if((_buffer[i] & 0x8) == 0) {
                    pos = i * 8 + 0x4;
                    break;
                } else if((_buffer[i] & 0x10) == 0) {
                    pos = i * 8 + 0x5;
                    break;
                } else if((_buffer[i] & 0x20) == 0) {
                    pos = i * 8 + 0x6;
                    break;
                } else if((_buffer[i] & 0x40) == 0) {
                    pos = i * 8 + 0x7;
                    break;
                } else if((_buffer[i] & 0x80) == 0) {
                    pos = i * 8 + 0x8;
                    break;
                }
            }
        }
        if(pos > _count)
            pos = 0;
        return pos;
    }
private:
    unsigned char* _buffer;
    int _count;
};

#endif
