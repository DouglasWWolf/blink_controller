#include <unistd.h>
#include <stdio.h>

#include "gpio_ctrl.h"

#include "blink_ctrl.h"
CBlinkCtrl ctrl;

CGpioCtrl gpio;

int main()
{
    ctrl.start(500);
    gpio.start("/dev/device", 3, "data");

    gpio.set(true, 300);

    sleep(40000);
    gpio.set(false);
    exit(1);


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

