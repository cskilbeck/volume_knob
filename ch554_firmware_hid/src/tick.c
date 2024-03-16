#include "main.h"

//////////////////////////////////////////////////////////////////////

#define TICK_PERIOD ((FREQ_SYS / 12) / TICK_FREQ)

//////////////////////////////////////////////////////////////////////

volatile uint8 tick = 0;

void timer2_irq_handler(void) __interrupt(INT_NO_TMR2)
{
    TF2  = 0;
    tick = 1;
}

//////////////////////////////////////////////////////////////////////

void tick_init()
{
    // switch off T2
    TR2 = 0;

    // set T2 counter reload value
    RCAP2 = (uint16)(65535U - TICK_PERIOD);

    // reset T2 counter
    T2COUNT = 0;

    // clear T2 overflow flag
    TF2 = 0;

    // enable T2 irq
    ET2 = 1;

    // switch on T2
    TR2 = 1;
}

//////////////////////////////////////////////////////////////////////
// wait for next tick (up to 1ms later - might be very soon...)

void tick_wait()
{
    tick = 0;
    while(!tick) {
    }
}
