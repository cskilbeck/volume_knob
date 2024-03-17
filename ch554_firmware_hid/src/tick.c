#include "main.h"

//////////////////////////////////////////////////////////////////////

#define TICK_PERIOD ((FREQ_SYS / 12) / TICK_FREQ)

//////////////////////////////////////////////////////////////////////

void tick_init()
{
    // set T2 counter reload value
    RCAP2 = (uint16)(65535U - TICK_PERIOD);

    // reset T2 counter
    T2COUNT = 0;

    // clear T2 overflow flag
    TF2 = 0;

    // switch on T2
    TR2 = 1;
}

//////////////////////////////////////////////////////////////////////
// wait for next tick (up to 1ms later - might be very soon...)

void tick_wait()
{
    TF2 = 0;
    while(!TF2) {
    }
}
