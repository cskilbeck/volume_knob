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
    // setup reload value
    RCAP2 = (uint16)(65535U - TICK_PERIOD);

    T2COUNT = 0;

    // clear overflow flag
    TF2 = 0;

    // enable irq
    ET2 = 1;

    // switch on T0
    TR2 = 1;
}
