#include <bitset>

class CBlinkCtrl
{
public:

    // Start the thread that managed blinking
    void    start(uint32_t blink_period_ms);

    // Start blinking the specified LED
    void    start_blink(unsigned int index);

    // Stop blinking the specified LED
    void    stop_blink(unsigned int index);

    // Stop blinking all LEDs
    void    clear();

protected:

    void    task();

    void    blink_now();

    std::bitset<64> m_blinkers;

    int             m_blinker_count;

    uint32_t        m_blink_period_sec, m_blink_period_usec;

    int             m_pipe[2];
};
