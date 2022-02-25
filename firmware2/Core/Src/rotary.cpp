#include "main.h"
#include "rotary.h"
#include "user.h"

int rotary_delta = 0;

//////////////////////////////////////////////////////////////////////
// Valid transitions are:
// 1    00 .. 01
// 2    00 .. 10
// 4    01 .. 00
// 7    01 .. 11
// 8    10 .. 00
// 11   10 .. 11
// 13   11 .. 01
// 14   11 .. 10

// bitmask of which 2-state histories are valid (see table above)
#define valid_state_mask 0x6996

// then, to just get one increment per cycle:

// 11 .. 10 .. 00 is one way
// 00 .. 10 .. 11 is the other way

// So:
// E8 = 11,10 .. 10,00  --> one way
// 2B = 00,10 .. 10,11  <-- other way

static int state = 0;
static uint8 store = 0;

void read_encoder()
{
    int a = ((ROTARYA_GPIO_Port->IDR & ROTARYA_Pin) != 0) ? 1 : 0;
    int b = ((ROTARYB_GPIO_Port->IDR & ROTARYB_Pin) != 0) ? 2 : 0;

    state = ((state << 2) | (a | b)) & 0xf;

    // many states are invalid (noisy switches) so ignore them
    if((valid_state_mask & (1 << state)) != 0) {
        // certain state patterns mean rotation happened
        store = (store << 4) | state;
        switch(store) {
        case 0xe8:
            rotary_delta += 1;
            break;
        case 0x2b:
            rotary_delta -= 1;
        }
    }
}
