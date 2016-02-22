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
#include <string>
#include <dirent.h>
#include "logger.h"

void Usage(const char* name) {
	printf("Usage: %s -f [config file] -l [log file]\n",name);
}

bool g_running = true;

int getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

ServerConfig g_serverConfig;

void signal_handler(int sig) {
    printf("Boat Server Recvive Ctrl-C signal\n");
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

inline int is_file_exist(const char* filepath) {
	if(filepath == NULL)
		return -1;
	if(access(filepath,F_OK|R_OK) == 0)
	 	return 0;
	return -1;
}

inline int is_dir_exist(const char* dirpath) {
	if(dirpath == NULL)
		return -1;
	if(opendir(dirpath) == NULL)
		return -1;
	return 0;
}

int main(int args,char** argv) {
	char configFile[128] = {0};
	char logFile[256] = {0};
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
	int ch;

    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("Could not register signal handler\n");
        exit(1);
    }

	while((ch = getopt(args,argv,"f:l:")) != -1) {
        switch(ch) {
            case 'f':
                strcpy(configFile,optarg);
                break;
			case 'l':
				strcpy(logFile,optarg);
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
		int log_level = 0;
		Logger* logger = NULL;

		if(!g_serverConfig.init(configFile)||!g_serverConfig.parseData()) {
            printf("Open Or parse config error,please check\n");
            break;
        }
		
		if(logFile[0] == 0) {
			g_serverConfig.getString("log_path",logFile);
		}

		if(logFile[0] != 0) {
			Logger::setInstaceFile(logFile);
		}
		
		logger = Logger::getInstance();

		if(logger == NULL) {
			printf("Create OR Find File Error,please check!\n");
			break;
		}
		
		logger->info(9,"[Version] " BOATSERVERVER);	
		g_serverConfig.debug();

		g_serverConfig.getInt("log_level",&log_level);

		logger->setLogLevel(log_level);

        if(!g_serverConfig.getString("hardware_com1",serial1)) {
            logger->error("[main] Do not have serial port 1 info");
            break;
        }

		if(!g_serverConfig.getString("hardware_com2",serial2)) {
            logger->error("[main] Do not have serial port 2 info");
            break;
        }

		if(!g_serverConfig.getString("autocontrol_lua",autocontrol_script)) {
            logger->error("[main] Do not have auto control script");
            break;
        }

		if(!g_serverConfig.getString("script_path",script_path)) {
            logger->error("[main] Do not have script recv path");
            break;
        }

		if(!g_serverConfig.getInt("server_port",&port)) {
            logger->error("[main] Do not have script port");
            break;
        }

		if(is_dir_exist(script_path) != 0) {
			logger->error("[main] Do not have this work dir %s",script_path);
			break;
		}

		std::string filename = std::string(script_path) + "/" + autocontrol_script;

		if(is_file_exist(filename.c_str()) != 0) {
			logger->error("[main] Do not have auto run script %s.",filename.c_str());
			break;
		}

		l_hardware = new Hardware(serial1,serial2);

		if(l_hardware->openHardware() == false) {
			logger->error("[main] Hardware init failed");
			break;
		}

		l_pilot = new Pilot(l_hardware,filename.c_str());

		l_server = new NetServer(port,l_pilot,script_path);

		if(l_server->init() == false) {
			logger->error("[main] Net Server init failed");
			break;
		}

		l_data.hardware = l_hardware;
		l_data.server = l_server;

		if (0 != pthread_create(&l_worker, NULL, worker_thread, (void*)&l_data)) {
			logger->error("[main] Create another thread failed");
			break;
		}

        while(g_running) {
            l_pilot->handleControl();
        }

		pthread_join(l_worker,NULL);
		l_server->closeServer();
		l_hardware->closeHardware();

		delete l_server;
		delete l_pilot;
		delete l_hardware;
		
		Logger::destroyInstance();
    }while(0);

    return 0;
}
