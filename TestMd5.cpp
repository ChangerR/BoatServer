#include "MD5.h"
#include <stdio.h>

int main(int args,char** argv) {
    if(args != 2) {
        printf("error we need filename\n");
        return 1;
    }
    const char* filename = argv[1];
    unsigned char out[16] = {0};

    MD5::md5sum(filename,out);

    printf("md5 %X%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X\n",out[0],out[1],out[2],out[3]
													,out[4],out[5],out[6],out[7]
													,out[8],out[9],out[10],out[11]
												,out[12],out[13],out[14],out[15]);
                                                
    return 0;
}
