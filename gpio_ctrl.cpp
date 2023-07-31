#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <thread>
#include "gpio_ctrl.h"


//=====================================================================================================================
// These are fake gpiod functions for compiling on X86
//=====================================================================================================================
#if 1
gpiod_chip* gpiod_chip_open_by_name(const char* chip_name)                           {return (gpiod_chip*)1;}
gpiod_line* gpiod_chip_get_line(gpiod_chip* chip, unsigned int gpio_num)             {return (gpiod_line*)1;}
int  gpiod_line_request_output(gpiod_line* line, const char* tag, int default_state) {return 0;}
void gpiod_line_release(gpiod_line* line)                                            {}
void gpiod_chip_close(gpiod_chip* chip)                                              {}

int gpiod_line_set_value(gpiod_line* line, int state)
{
    printf("LED %s\n", state ? "ON" : "OFF");    
    return 0;
}
#endif
//=====================================================================================================================

static volatile int bitbucket;


//=====================================================================================================================
// This is the message that passes between thread
//=====================================================================================================================
struct msg_t {char state; unsigned int period_ms;};
//=====================================================================================================================


//=====================================================================================================================
// start() - Initialized hardware and starts the management thread
//=====================================================================================================================
bool CGpioCtrl::start(const char* chip_name, unsigned int gpio_num, const char* tag)
{
    // Open the chip device
    m_chip = gpiod_chip_open_by_name(chip_name);
    
    // If that failed, complain
    if (m_chip == nullptr)
    {
        fprintf(stderr, "Failed to open %s\n", chip_name);
        return false;
    }

    // Get a reference to the GPIO line we care about
    m_line = gpiod_chip_get_line(m_chip, gpio_num);

    // If that failed, complain
    if (m_line == nullptr)
    {
        close();
        fprintf(stderr, "gpiod_chip_get_line() failed for GPIO %d\n:", gpio_num);
        return false;
    }

    // Make the requested GPIO line an output
    int rc = gpiod_line_request_output(m_line, tag, 0);

    // If that failed, complain
    if (rc < 0)
    {
        close();
        fprintf(stderr, "gpiod_line_request_output() failed for GPIO %d\n:", gpio_num);
        return false;
    }
    
    // Create the pipe that the threads will use to communicate
    rc = pipe2(m_pipe, O_CLOEXEC);

    // Launch task() in a separate thread
    std::thread th(&CGpioCtrl::task, this);
    th.detach();

    // Tell the caller that all is well
    return true;
}
//=====================================================================================================================


//=====================================================================================================================
// close() - Closes the GPIOD device
//=====================================================================================================================
void CGpioCtrl::close()
{
    if (m_line) gpiod_line_release(m_line);
    m_line = nullptr;
    if (m_chip) gpiod_chip_close(m_chip);
    m_chip = nullptr;
}
//=====================================================================================================================


//=========================================================================================================
// task() - This routine runs as an independent thread and managed the state of the GPIO
//=========================================================================================================
void CGpioCtrl::task()
{
    int      gpio_state = 0;
    fd_set   rfds;
    timeval  timeout, *p_timeout;
    bool     blinking = false;
    uint32_t blink_period_sec = 0, blink_period_usec = 0;
    msg_t    msg;

    // Get a handy name for the file descriptor we'll read from
    int fd = m_pipe[0];

    // We're going to sit in this loop forever
    while (true)
    {
        // Create the fd_set of descriptors we're going to wait on
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);        

        // Fill in the timeout structure with the appropriate seconds/microseconds
        timeout.tv_sec  = blink_period_sec;
        timeout.tv_usec = blink_period_usec;

        // The timeout is either the specified timeout or "wait forever"
        p_timeout = (blinking) ? &timeout : nullptr;

        // Wait for a command to arrive.  If we time out, it means we need to blink
        if (select(fd+1, &rfds, nullptr, nullptr, p_timeout) == 0)
        {
            gpio_state = 1-gpio_state;
            if (m_line) gpiod_line_set_value(m_line, gpio_state);
            continue;
        }

        // Read the incoming message
        bitbucket = read(fd, &msg, sizeof(msg));

        // Fetch the new state of the GPIO from the message
        gpio_state = msg.state;
        
        // Set the physical output line to match this new state
        if (m_line) gpiod_line_set_value(m_line, gpio_state);

        // Is this GPIO blinking?
        blinking = (msg.period_ms != 0);
        
        // If this GPIO will blink, compute the blink period 
        if (blinking)
        {
            // Convert the blink period from msec to usec
            uint32_t usec = msg.period_ms * 1000;

            // Save the blink period
            blink_period_sec  = usec / 1000000;
            blink_period_usec = usec % 1000000;
        }
    }
}
//=========================================================================================================


//=========================================================================================================
// set() - Turns the GPIO on or off, with an option blink period
//=========================================================================================================
void CGpioCtrl::set(bool state, unsigned int period_ms)
{
    msg_t msg = {state, period_ms};
    bitbucket = write(m_pipe[1], &msg, sizeof(msg));
}
//=========================================================================================================
