#ifndef _BOATSERVER_MD5_H
#define _BOATSERVER_MD5_H

#define MD5_DIGEST_SIZE		16
#define MD5_HMAC_BLOCK_SIZE	64
#define MD5_BLOCK_WORDS		16
#define MD5_HASH_WORDS		4

#define MD5_H0	0x67452301UL
#define MD5_H1	0xefcdab89UL
#define MD5_H2	0x98badcfeUL
#define MD5_H3	0x10325476UL

class MD5 {
public:
    MD5(){};
    virtual ~MD5(){};

    void init();

    int update(const unsigned char* data,unsigned int len);

    int final();

    void printMD5();

    const unsigned char* getMD5() {
        return (unsigned char*)_hash;
    }

    static bool md5sum(const char* filename,unsigned char* out);
private:
    static unsigned char _md5_zero_message_hash[MD5_DIGEST_SIZE];
    unsigned int _hash[MD5_HASH_WORDS];
    unsigned int _block[MD5_BLOCK_WORDS];
    unsigned long long _byte_count;
};

#endif
