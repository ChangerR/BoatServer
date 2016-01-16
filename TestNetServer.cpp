#include <stdio.h>
#include "NetServer.h"
#include "Pilot.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

bool g_running = true;

void signal_handler(int sig) {
    printf("Ctrl-C signal\n");
    g_running = false;
}

int main(int args,char** argv) {
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("could not register signal handler\n");
        exit(1);
    }
    Pilot* pilot = new Pilot(NULL);
    NetServer* server = new NetServer(5000,pilot);

    if(server->init()) {
        while(g_running) {
            server->handleServerMessage();
            pilot->handleControl();
        }
        usleep(1);
    }
    server->closeServer();
    delete server;
    delete pilot;
    return 0;
}
