#include "MD5.h"
#include <stdio.h>

unsigned char g_buffer[4096] = {0};
int main(int args,char** argv) {
    if(args != 2) {
        printf("error we need filename\n");
        return 1;
    }
    const char* filename = argv[1];
    MD5 md5;
    FILE* f = fopen(filename,"rb");

    if(f == NULL) {
        return 1;
    }
    fseek(f,0,SEEK_END);
    int filelen = ftell(f);
    fseek(f,0,SEEK_SET);
    md5.init();
    do{
        int readlen = filelen > 4096 ? 4096 : filelen;
        fread(g_buffer,readlen,1,f);
        md5.update(g_buffer,readlen);
        filelen -= readlen;
    }while (filelen > 0);

    fclose(f);
    md5.final();
    md5.printMD5();
    return 0;
}
