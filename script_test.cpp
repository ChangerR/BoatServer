#include "AutoController-mock.h"
#include <stdio.h>
#include "logger.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

void Usage(const char* name) {
	printf("Usage: %s -f [script] -t [times]\n",name);
}

int main(int args,char** argv) {
	int ch;
	char script[256] = {0};
	char c_times[64] = {0};
	int times = 0;
	printf("times=%d\n",args);
	if(args == 1) {
		Usage(argv[0]);
		return 1;
	}

	while((ch = getopt(args,argv,"f:t:")) != -1) {
        switch(ch) {
            case 'f':
                strcpy(script,optarg);
                break;
			case 't':
				strcpy(c_times,optarg);
				break;
            default:
                break;
        }
    }
	
	if(script[0] == 0||c_times[0] == 0) {
		Usage(argv[0]);
		return 1;
	}

	times = parse_int_dec(c_times);
	AutoControllerMock contorl;

	if(!contorl.init(script))
		return 1;

	for(int i = 0; i < times;i++) {
		contorl.runController();
	}

	return 0;
}
