#include "main.h"

//////////////////////////////////////////////////////////////////////
// Pn_MOD_OC & Pn_DIR_PU: pin input & output configuration for Pn (n=1/3)
//   0 0:  float input only, without pullup resistance
//   0 1:  push-pull output, strong driving high level and low level
//   1 0:  open-drain output and input without pullup resistance
//   1 1:  quasi-bidirectional (standard 8051 mode), open-drain output and input
//   with pullup resistance, just driving high level strongly for 2 clocks if
//          turning output level from low to high

//////////////////////////////////////////////////////////////////////

void gpio_init(uint8 port, uint8 bit, gpio_state_t state)
{
    uint8 mask_on = 1 << bit;
    uint8 mask_off = ~mask_on;
    uint8 oc_mask = 0;
    uint8 pu_mask = 0;
    if(state & 2) {
        oc_mask = mask_on;
    }
    if(state & 1) {
        pu_mask = mask_on;
    }

    switch(port) {
    case PORT1:
        P1_MOD_OC = (P1_MOD_OC & mask_off) | oc_mask;
        P1_DIR_PU = (P1_DIR_PU & mask_off) | pu_mask;
        break;
    case PORT3:
        P3_MOD_OC = (P3_MOD_OC & mask_off) | oc_mask;
        P3_DIR_PU = (P3_DIR_PU & mask_off) | pu_mask;
        break;
    }
}
