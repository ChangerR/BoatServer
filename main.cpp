#include "serial.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"
#include <string.h>
#include "slconfig.h"
#include "ServerConfig.h"
#include "Pilot.h"
#include "NetServer.h"
#include "Hardware.h"
#include <pthread.h>

void Usage(const char* name) {
	printf("Usage: %s -f [config file]\n",name);
}

bool g_running = true;

int getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

ServerConfig g_serverConfig;

void signal_handler(int sig) {
    printf("Ctrl-C signal\n");
    g_running = false;
}

struct private_data {
	Hardware* hardware;
	NetServer* server;
};

void* __cdecl worker_thread(void* data) {
	private_data* l_data = (private_data*)data;
	Hardware* hardware = l_data->hardware;
	NetServer* server = l_data->server;
	
	while(g_running) {
		server->handleServerMessage();
		hardware->processHardwareMsg();
	}
	return NULL;
}

int main(int args,char** argv) {
	char configFile[128] = {0};
	char serial1[64] = {0};
	char serial2[64] = {0};
	char autocontrol_script[64] = {0};
	char script_path[256] = {0};
	Hardware* l_hardware = NULL;
	NetServer* l_server = NULL;
	Pilot* l_pilot = NULL;
	private_data l_data;
	int port = 0;
	pthread_t l_worker;
	
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

        if(!g_serverConfig.getString("hardware_com1",serial1)) {
            printf("do not have serial port 1 info\n");
            break;
        }
		
		if(!g_serverConfig.getString("hardware_com2",serial2)) {
            printf("do not have serial port 2 info\n");
            break;
        }
		
		if(!g_serverConfig.getString("autocontrol_lua",autocontrol_script)) {
            printf("do not have auto control script\n");
            break;
        }
		
		if(!g_serverConfig.getString("script_path",script_path)) {
            printf("do not have script recv path\n");
            break;
        }
		
		if(!g_serverConfig.getInt("server_port",&port)) {
            printf("do not have script port\n");
            break;
        }
		
		l_hardware = new Hardware(serial1,serial2);
		
		if(l_hardware.openHardware() == false) {
			printf("hardware init failed\n");
			break;
		}

		l_pilot = new Pilot(l_hardware,autocontrol_lua);
		
		l_server = new NetServer(port,l_pilot,script_path);
		
		if(l_server.init() == false) {
			printf("server init failed\n");
			break;
		}
		
		l_data.hardware = l_hardware;
		l_data.server = l_server;
		
		if (0 != pthread_create(&l_worker, NULL, worker_thread, (void*)&l_data)) {
			printf("create another thread failed\n");
			break;
		} 
		
        while(g_running) {
            l_pilot->handleControl();
        }
		
		pthread_join(&l_worker,NULL);
		l_server->closeServer();
		l_hardware->closeHardware();
		
		delete l_server;
		delete l_pilot;
		delete l_hardware;

    }while(0);
	
    return 0;
}
