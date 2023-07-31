#pragma once
#include "gpiod.h"

class CGpioCtrl
{
public:

    // Constructor
    CGpioCtrl() {m_chip = nullptr; m_line = nullptr;}
   
    // Destructor
    ~CGpioCtrl() {close();}

    // Call this to start the management thread    
    bool start(const char* chip_name, unsigned int gpio_num, const char* tag);

    // Turn the GPIO on, or off, or make it blink
    void set(bool state, unsigned int period = 0);

    // Manually closes the GPIO device
    void close();

protected:

    // This is the thread that manages the GPIO
    void    task();

    // GPIOD hardware control structures
    gpiod_chip* m_chip;
    gpiod_line* m_line;

    // The pipe for communicating with the management thread
    int m_pipe[2];


};



