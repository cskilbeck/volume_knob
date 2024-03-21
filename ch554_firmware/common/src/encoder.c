#include "main.h"

//////////////////////////////////////////////////////////////////////

static __code uint8 const encoder_valid_bits[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

static uint8 encoder_state = 0;
static uint8 encoder_store = 0;

//////////////////////////////////////////////////////////////////////

void encoder_init()
{
    gpio_init(ROTA_PORT, ROTA_PIN, gpio_input_pullup);
    gpio_init(ROTB_PORT, ROTB_PIN, gpio_input_pullup);
    gpio_init(BTN_PORT, BTN_PIN, gpio_input_pullup);
}

//////////////////////////////////////////////////////////////////////

#if DEVICE == DEVICE_DEVKIT
#define ROTATE_CW 1
#define ROTATE_CWW -1
#else
#define ROTATE_CW -1
#define ROTATE_CWW 1
#endif

int8 encoder_read()
{
    uint8 a = 0;
    if(!ROTA_BIT) {
        a |= 1;
    }
    if(!ROTB_BIT) {
        a |= 2;
    }

    encoder_state <<= 2;
    encoder_state |= a;
    encoder_state &= 0xf;

    if(encoder_valid_bits[encoder_state] != 0) {
        encoder_store = (encoder_store << 4) | encoder_state;
        switch(encoder_store) {
        case 0xe8:
            return ROTATE_CW;
        case 0x2b:
            return ROTATE_CWW;
        }
    }
    return 0;
}

// odd # of bits set (1 or 3) means it's valid

// 0  00 00 0
// 1  00 01 1
// 2  00 10 1
// 3  00 11 0
// 4  01 00 1
// 5  01 01 0
// 6  01 10 0
// 7  01 11 1
// 8  10 00 1
// 9  10 01 0
// A  10 10 0
// B  10 11 1
// C  11 00 0
// D  11 01 1
// E  11 10 1
// F  11 11 0
