//////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdbool.h>
#include <ch554.h>
#include <debug.h>
#include "hid.h"
#include "types.h"
#include "gpio.h"
#include "hid_keys.h"

#define CLOCKWISE 2
#define ANTI_CLOCKWISE 0

// Define ROTARY_DIRECTION as CLOCKWISE for one kind of encoders, ANTI_CLOCKWISE for
// the other ones (some are reversed). This sets the default rotation (after first
// flash), reverse by triple-clicking the knob

#define ROTARY_DIRECTION (CLOCKWISE)
// #define ROTARY_DIRECTION (ANTI_CLOCKWISE)

//////////////////////////////////////////////////////////////////////
// how long a button press goes into bootloader

#define BOOTLOADER_DELAY 0x300    // about 3 seconds

//////////////////////////////////////////////////////////////////////

typedef int8_t int8;
typedef int16_t int16;
typedef uint8_t uint8;
typedef uint16_t uint16;

//////////////////////////////////////////////////////////////////////
// BOOTLOADER admin

typedef void (*BOOTLOADER)(void);
#define bootloader554 ((BOOTLOADER)0x3800)    // CH551/2/3/4
#define bootloader559 ((BOOTLOADER)0xF400)    // CH558/9

//////////////////////////////////////////////////////////////////////

#define MEDIA_KEY(x) ((x) | 0x8000)
#define NORMAL_KEY(x) (x)

//////////////////////////////////////////////////////////////////////

void hard_fault()
{
    while(1) {
        LED_BIT ^= 1;
        for(uint16_t i = 1; i != 0; ++i) {
        }
    }
}

//////////////////////////////////////////////////////////////////////
// rotary encoder reader

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

uint8 const encoder_valid_bits[16] = { 0, 1, 1, 0, 1, 0, 0, 1,
                                       1, 0, 0, 1, 0, 1, 1, 0 };

uint8 encoder_state = 0;
uint8 encoder_store = 0;

int8 read_encoder()
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
            return 1;
        case 0x2b:
            return -1;
        }
    }
    return 0;
}

// uint8 last_a;

// int8 read_encoder()
// {
//     uint8 a = !ROTA_BIT;
//     int8 result = 0;
//     if(a != last_a) {
//         if(!ROTB_BIT != a) {
//             result = 1;
//         } else {
//             result = -1;
//         }
//         last_a = a;
//     }
//     return result;
// }

//////////////////////////////////////////////////////////////////////
// Flash LED before jumping to bootloader

void bootloader_led_flash(int8 n)
{
    LED_BIT = 0;
    for(int8 i = 0; i < n; ++i) {
        TF2 = 0;
        TH2 = 0;
        TL2 = 120;
        while(TF2 != 1) {
        }
        LED_BIT ^= 1;
    }
}

//////////////////////////////////////////////////////////////////////
// led pulse/fade thing to indicate rotary/button activity

uint8 const led_gamma[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
    2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,
    5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,
    10,  10,  11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,
    17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,
    25,  26,  27,  27,  28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  35,  36,
    37,  38,  39,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  50,
    51,  52,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  66,  67,  68,
    69,  70,  72,  73,  74,  75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,
    90,  92,  93,  95,  96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

uint16 led_brightness = 0;

#define LED_FADE_SPEED 3

void led_update()
{
    if(led_brightness <= LED_FADE_SPEED) {
        LED_BIT = 1;
    } else {
        led_brightness -= LED_FADE_SPEED;
        LED_BIT = led_gamma[led_brightness >> 8] <= TH0;
    }
}

inline void led_flash()
{
    led_brightness = 65535u;
}

//////////////////////////////////////////////////////////////////////
// read bytes from the data flash area

void read_flash_data(uint8 flash_addr, uint8 len, uint8 *data)
{
    flash_addr <<= 1;

    while(len-- != 0) {

        ROM_ADDR_L = flash_addr;
        ROM_ADDR_H = DATA_FLASH_ADDR >> 8;

        ROM_CTRL = ROM_CMD_READ;

        if((ROM_STATUS & bROM_CMD_ERR) != 0) {
            break;
        }

        *data++ = ROM_DATA_L;

        flash_addr += 2;
    }
}

//////////////////////////////////////////////////////////////////////
// write bytes to the data flash area

void write_flash_data(uint8 flash_addr, uint8 len, uint8 *data)
{
    flash_addr <<= 1;

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;

    GLOBAL_CFG |= bDATA_WE;

    while(len-- != 0) {

        ROM_ADDR_L = flash_addr;
        ROM_ADDR_H = DATA_FLASH_ADDR >> 8;

        ROM_DATA_L = *data++;

        ROM_CTRL = ROM_CMD_WRITE;

        if((ROM_STATUS & bROM_CMD_ERR) != 0) {
            break;
        }

        flash_addr += 2;
    }

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;

    GLOBAL_CFG &= ~bDATA_WE;
}

//////////////////////////////////////////////////////////////////////
// a queue of key states to send

// must be a power of 2
#define KEY_QUEUE_LEN 16

uint16 queue_buffer[KEY_QUEUE_LEN];
uint8 queue_head = 0;
uint8 queue_size = 0;

inline bool queue_full()
{
    return queue_size == KEY_QUEUE_LEN;
}

inline uint8 queue_space()
{
    return KEY_QUEUE_LEN - queue_size;
}

inline bool queue_empty()
{
    return queue_size == 0;
}

// check it's got space before calling this

void queue_put(uint16 k)
{
    queue_buffer[(queue_head + queue_size) & (KEY_QUEUE_LEN - 1)] = k;
    queue_size += 1;
}

// check it's not empty before calling this

uint16 queue_get()
{
    uint16 next = queue_buffer[queue_head];
    queue_size -= 1;
    queue_head = ++queue_head & (KEY_QUEUE_LEN - 1);
    return next;
}

//////////////////////////////////////////////////////////////////////
// add a momentary keypress to the queue and flash the led

void do_press(uint16 k)
{
    if(queue_space() > 1) {
        queue_put(k);
        queue_put(k & 0x8000);
        led_flash();
    } else {
        led_flash();
    }
}

//////////////////////////////////////////////////////////////////////

#define DEBOUNCE_TIME 0x1Cu
#define T2_DEBOUNCE (0xFFu - DEBOUNCE_TIME)

uint16 press_time = 0;

uint8 vol_direction;
int8 turn_value;

void main()
{
    // init clock
    CfgFsys();

    read_flash_data(0, 1, &vol_direction);

    if(vol_direction == 0xff) {
        vol_direction = ROTARY_DIRECTION;
    }
    turn_value = (int8)vol_direction - 1;    // becomes -1 or 1

#if DEVICE == DEVICE_DEVKIT
    gpio_init(UART_TX_PORT, UART_TX_PIN, gpio_output_push_pull);
    gpio_init(UART_RX_PORT, UART_RX_PIN, gpio_output_open_drain);
#endif
    gpio_init(ROTA_PORT, ROTA_PIN, gpio_input_pullup);
    gpio_init(ROTB_PORT, ROTB_PIN, gpio_input_pullup);
    gpio_init(BTN_PORT, BTN_PIN, gpio_input_pullup);
    gpio_init(LED_PORT, LED_PIN, gpio_output_push_pull);

    // start usb client
    usb_init();

    // Start Timer0, Timer1, Timer2
    TR0 = 1;
    TR1 = 1;
    TR2 = 1;

    bootloader_led_flash(7);

    // Triple click admin
    uint8 t1_count = 0;
    uint8 clicks = 0;

    // flash led slowly until USB is connected (in case of power-only cable)

    {
        uint8_t flash = 0;

        while(!usb_active) {

            if(TF2 == 1) {
                TL2 = 0;
                TH2 = 0;
                TF2 = 0;
                flash += 1;
                LED_BIT = (flash >> 4) & 1;
            }
        }
    }

    // main loop

    bool button_state = false;    // for debouncing the button

    while(1) {

        // read/debounce the button
        bool pressed = false;
        bool new_state = !BTN_BIT;
        if(new_state != button_state && TF2 == 1) {
            TL2 = 0;
            TH2 = T2_DEBOUNCE;
            TF2 = 0;
            pressed = new_state;
            button_state = new_state;
        }

        // hold button down to go into bootloader
        if(button_state) {
            if(TF0 == 1) {
                TF0 = 0;
                press_time += 1;
                if(press_time == BOOTLOADER_DELAY) {

                    // shutdown peripherals
                    EA = 0;
                    USB_CTRL = 0;
                    UDEV_CTRL = 0;

                    // flash LED for a bit
                    bootloader_led_flash(8);

                    // and jump to bootloader
                    bootloader554();
                }
            }
        } else {
            press_time = 0;
        }

        // delay counter for knob triple-click
        if(TF1 == 1) {
            TF1 = 0;
            if(t1_count < 50) {
                t1_count += 1;
            } else {
                clicks = 0;
            }
        }

        // read the rotary encoder (returns -1, 0 or 1)
        int8 direction = read_encoder();

        // Timer2 does double duty, debounces encoder as well
        if(direction != 0 && TF2 == 1) {
            TL2 = 0;
            TH2 = T2_DEBOUNCE;
            TF2 = 0;
        } else {
            direction = 0;
        }

        // check for triple-click
        if(pressed) {
            if(t1_count < 50) {
                clicks += 1;
                if(clicks == 2) {
                    vol_direction = 2 - vol_direction;
                    turn_value = (int8)vol_direction - 1;
                    write_flash_data(0, 1, &vol_direction);
                    pressed = false;
                }
            }
            TL1 = 0;
            TH1 = 0;
            TF1 = 0;
            t1_count = 0;
        }

        // queue up some keypresses if something happened
        if(pressed) {

            do_press(MEDIA_KEY(KEY_MEDIA_MUTE));
        }

        if(direction == turn_value) {

            do_press(MEDIA_KEY(KEY_MEDIA_VOLUMEUP));

        } else if(direction == -turn_value) {

            do_press(MEDIA_KEY(KEY_MEDIA_VOLUMEDOWN));
        }

        // send key on/off to usb hid if there are some waiting to be sent
        if(usb_idle == 3 && !queue_empty()) {

            usb_set_keystate(queue_get());
        }
        led_update();
    }
}
