#include "Hardware.h"
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
    Hardware* hardware = new Hardware("/dev/ttyACM0","/dev/ttyACM1");

    if(hardware->openHardware()) {
        hardware->requestStatus();
        while(g_running) {
            hardware->processHardwareMsg();
            if(hardware->getStatus()->isAllUpdated()) {
                HWStatus* status = hardware->getStatus();
                printf("Recv hardware status laser1=%d laser2=%d laser3=%d laser4=%d laser5=%d "
                "longitude=%.3f latitude=%.3f height=%.3f speed=%.3f time=%.3f "
                "roll=%.3f pitch=%.3f yaw=%.3f\n",status->laser1,status->laser2,status->laser3,status->laser4,status->laser5,
                status->longitude,status->latitude,status->height,status->speed,status->time,
                status->roll,status->pitch,status->yaw);

                hardware->requestStatus();
            }
        }
    }
    hardware->closeHardware();
    delete hardware;
    return 0;
}
