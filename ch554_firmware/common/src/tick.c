#include "main.h"

//////////////////////////////////////////////////////////////////////

#define TICK_PERIOD ((FREQ_SYS / 12) / TICK_FREQ)

//////////////////////////////////////////////////////////////////////

uint16 ticks;

//////////////////////////////////////////////////////////////////////

void tick_init()
{
    ticks = 0;

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
// wait for N ticks

void tick_wait(uint16 n)
{
    for(uint16 i = 0; i < n; ++i) {
        TF2 = 0;
        while(!TF2) {
        }
    }
}
