#include "serial.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"
#include <string.h>
#include "slconfig.h"
#include "ServerConfig.h"

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
    
    return 0;
}
