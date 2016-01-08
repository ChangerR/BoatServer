#include "serial.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"
#include <string.h>
#include "slconfig.h"
#include "ServerConfig.h"
#include "Command.h"

bool g_running = true;
int getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

ServerConfig g_serverConfig;

void Usage(const char* name) {
	printf("Usage: %s -f [config file]\n",name);
}

void signal_handler(int sig) {
    printf("Ctrl-C signal\n");
    g_running = false;
}

int a = 1,b = 2;

int log_e(int args,char (*argv)[MAX_CMD_ARGUMENT_LEN],void* user) {
    printf("Log:%s\n",argv[0]);
    return 0;
}

int addr(int args,char (*argv)[MAX_CMD_ARGUMENT_LEN],void* user) {
    Serial* s = (Serial*)user;
    a = b;
    b = parse_int_dec(argv[0]);

    s->println("add(%d,%d)",a,b);
    return 0;
}

int main(int args,char** argv) {
    int ch = 0;
    char configFile[64] = {0};
    char buffer[256] = {0};
    char serial_port[256] = {0};
    Serial* serial = NULL;
    CmdEvent event;
    Command* cmd = new Command(&event);
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("could not register signal handler\n");
        exit(1);
    }

    while((ch = getopt(args,argv,"f:")) != -1) {
        switch(ch) {
            case 'f':
                strcpy(configFile,optarg);
                break;
            default:
                break;
        }
    }

    if(*configFile == 0) {
        Usage(argv[0]);
        exit(1);
    }

    do {
        if(!g_serverConfig.init(configFile)||!g_serverConfig.parseData()) {
            printf("Open Or parse config error,please check\n");
            break;
        }

        g_serverConfig.debug();

        if(!g_serverConfig.getString("hardware_com1",serial_port)) {
            printf("do not have serial port info\n");
            break;
        }
        serial = new Serial(serial_port);

        if(!serial->touchForCDCReset()||!serial->begin(_B115200)) {
            printf("Open Serial %s failed,Please Check\n", serial_port);
            break;
        }
        sleep(1);

        event.addEvent("log",log_e,NULL);
        event.addEvent("addr",addr,serial);

        serial->println("add(%d,%d)",a,b);

        while(g_running) {

            if(serial->available() > 0&&serial->readline(buffer,256) > 0) {
                //printf("Serial Return:%s\n",buffer);
                cmd->parseWithFire(buffer);
                /*
                if(!strncmp("Method Add Return:",buffer,18)) {
                    int c = parse_int_dec(buffer + 18);
                    a = b;
                    b = c;
                    serial->println("add(%d,%d)",a,b);
                }
                */
            }
            sleep(1);
        }
        serial->close();

        delete serial;
    }while(0);

    return 0;
}
