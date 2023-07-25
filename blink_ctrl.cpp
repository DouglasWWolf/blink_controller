#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <thread>
#include "blink_ctrl.h"

static volatile int rc;
static const char zero[] = {0};
static const char one[]  = {1};

//=========================================================================================================
// start() - Performs initialization and starts up the blink-management thread 
//=========================================================================================================
void CBlinkCtrl::start(uint32_t blink_period_ms)
{
    // Convert the blink period from msec to usec
    uint32_t usec = blink_period_ms * 1000;

    // Save the blink period
    m_blink_period_sec  = usec / 1000000;
    m_blink_period_usec = usec % 1000000;

    // So far, nothing is blinking
    m_blinkers.reset();

    // So far, nothing is blinking
    m_blinker_count = 0;
    
    // Create the pipe that the threads will use to communicate
    rc = pipe2(m_pipe, O_CLOEXEC);

    // Launch task() in a separate thread
    std::thread th(&CBlinkCtrl::task, this);
    th.detach();
}
//=========================================================================================================



//=========================================================================================================
// start_blink() - Begin blinking the object with the specified index
//=========================================================================================================
void CBlinkCtrl::start_blink(unsigned int index)
{
    // If this blinker is already blinking, do nothing
    if (m_blinkers[index]) return;

    // Set the bit indicating this index is blinking
    m_blinkers.set(index, true);

    // If we need to turn on the blinking mechanism, tell the other thread
    if (++m_blinker_count == 1)
    {
        rc = write(m_pipe[1], one, 1);
    }
}
//=========================================================================================================


//=========================================================================================================
// start_blink() - Stop blinking the object with the specified index
//=========================================================================================================
void CBlinkCtrl::stop_blink(unsigned int index)
{
    // If this blinker is already off, do nothing
    if (m_blinkers[index] == 0) return;

    // Indicate that this index is no longer blinking
    m_blinkers.set(index, false);

    // If we need to turn off the blinking mechanism, tell the other thread
    if (--m_blinker_count == 0)
    {
        rc = write(m_pipe[1], zero, 1);
    }
}
//=========================================================================================================



//=========================================================================================================
// clear() - Stop blinking all objects
//=========================================================================================================
void CBlinkCtrl::clear()
{
    m_blinkers.reset();
    m_blinker_count = 0;
    rc = write(m_pipe[1], zero, 1);
}
//=========================================================================================================


//=========================================================================================================
// task() - This routine runs as an independent thread and is responsible for notifying the "blink now" 
//          logic when it's time to blink objects
//=========================================================================================================
void CBlinkCtrl::task()
{
    char    blinking = 0;
    fd_set  rfds;
    timeval timeout, *p_timeout;
    
    // Get a handy name for the file descriptor we'll read from
    int fd = m_pipe[0];

    // We're going to sit in this loop forever
    while (true)
    {
        // Create the fd_set of descriptors we're going to wait on
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);        

        // Fill in the timeout structure with the appropriate seconds/microseconds
        timeout.tv_sec  = m_blink_period_sec;
        timeout.tv_usec = m_blink_period_usec;

        // The timeout is either the specified timeout or "wait forever"
        p_timeout = (blinking) ? &timeout : nullptr;

        // Wait for a command to arrive.  If we time out, it means we need to blink
        if (select(fd+1, &rfds, NULL, NULL, p_timeout) == 0)
        {
            blink_now();
            continue;
        }

        // Read the new state of the blinking mechanism
        rc = read(fd, &blinking, 1);
    }
}
//=========================================================================================================


//=========================================================================================================
// blink_now() - Fill this in with the code that blinks your LEDs.   m_blinkers contains a bitmap of which
//               LEDs should be blinked.
//=========================================================================================================
void CBlinkCtrl::blink_now()
{
    // Just for demo
    printf("Blink LEDs");

    // Loop through each LED that requires blinking...
    for (int i=0; i<m_blinkers.size(); ++i) if (m_blinkers[i])
    {
        // And blink it!
        printf(" %i", i);
    }

    // This would be a good place to call "show()"
    printf("\n");

}
//=========================================================================================================
