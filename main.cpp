#include <unistd.h>
#include <stdio.h>

#include "blink_ctrl.h"
CBlinkCtrl ctrl;


int main()
{
    ctrl.start(500);

    printf("Starting...\n");
    ctrl.start_blink(1);
    ctrl.start_blink(3);
    sleep(6);
    
    printf("Stopping\n");
    ctrl.stop_blink(1);
    ctrl.stop_blink(3);
    sleep(4);

    printf("Starting...\n");
    ctrl.start_blink(2);
    sleep(6);


}

