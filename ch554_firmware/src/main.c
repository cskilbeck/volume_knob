//////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdbool.h>
#include <ch554.h>
#include <debug.h>
#include "hid.h"

//////////////////////////////////////////////////////////////////////

typedef int8_t int8;
typedef int16_t int16;
typedef uint8_t uint8;
typedef uint16_t uint16;

uint8 const gammaTable[256] = { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
                                1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,
                                4,   4,   4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,
                                13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,  25,  26,  27,  27,
                                28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  50,  51,
                                52,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  66,  67,  68,  69,  70,  72,  73,  74,  75,  77,  78,  79,  81,  82,  83,  85,  86,
                                87,  89,  90,  92,  93,  95,  96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133,
                                135, 137, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175, 177, 180, 182, 184, 186, 189, 191, 193,
                                196, 198, 200, 203, 205, 208, 210, 213, 215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255 };

//////////////////////////////////////////////////////////////////////
// GPIOs

#define PORT1 0x90
#define PORT3 0xB0

#define LED_PIN 7
#define BTN5_PIN 5

#define ROTA_PIN 3
#define ROTB_PIN 4

SBIT(LED_BIT, PORT1, LED_PIN);
SBIT(BTN5_BIT, PORT1, BTN5_PIN);

SBIT(ROTA_BIT, PORT3, ROTA_PIN);
SBIT(ROTB_BIT, PORT3, ROTB_PIN);

//////////////////////////////////////////////////////////////////////
// rotary encoder reader

uint8 const valid_bits[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

static uint8 state = 0;
static uint8 store = 0;

int8 rotary_delta = 0;

int8 read_encoder()
{
    uint8 a = 0;
    if(!ROTA_BIT) {
        a |= 1;
    }
    if(!ROTB_BIT) {
        a |= 2;
    }

    state <<= 2;
    state |= a;
    state &= 0xf;

    if(valid_bits[state] != 0) {
        store = (store << 4) | state;
        switch(store) {
        case 0xe8:
            return 1;
        case 0x2b:
            return -1;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////
// flash the led for n intervals (Timer0 wrap time)
// see end of main for the actual flashing bit

int16 led_brightness = 0;
int16 led_target = 255;
int8 led_delta = 1;

uint8 led_counter = 0;

void led_update()
{
    LED_BIT = TH0 > ((uint16)led_brightness >> 8);
    if(++led_counter == 255) {
        led_counter = 0;
        led_brightness += led_delta;
        if(led_delta < 0 && led_brightness < led_target) {
            led_delta = 0;
            led_brightness = led_target;
        }
        if(led_delta > 0 && led_brightness > led_target) {
            led_delta = 0;
            led_brightness = led_target;
        }
    }
}

// bright = initial brightness
// target = target brightness
// speed = how fast to go from bright to target

// eg: 255, 0, -1

inline void led_flash(uint8 bright, uint8 target, int8 speed)
{
    led_brightness = bright;
    led_target = target;
    led_delta = bright == target ? 0 : bright > target ? -speed : speed;
}

//////////////////////////////////////////////////////////////////////
// a queue of key states to send
// if usb is idle and the queue is not empty, send next one

// must be a power of 2
#define KEY_QUEUE_LEN 32

uint8 queue_buffer[KEY_QUEUE_LEN];
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

void queue_put(int k)
{
    queue_buffer[(queue_head + queue_size) & (KEY_QUEUE_LEN - 1)] = k;
    queue_size += 1;
}

// check it's not empty before calling this

uint8 queue_get()
{
    uint8 next = queue_buffer[queue_head];
    queue_size -= 1;
    queue_head = ++queue_head & (KEY_QUEUE_LEN - 1);
    return next;
}

//////////////////////////////////////////////////////////////////////
// add a momentary keypress to the queue and flash the led

void do_press(int k)
{
    if(queue_space() > 1) {
        queue_put(k);
        queue_put(0);
        led_flash(255, 0, -4);
    }
}

//////////////////////////////////////////////////////////////////////

#define DEBOUNCE_TIME 0x70u
#define T2_DEBOUNCE (0xFFu - DEBOUNCE_TIME)

void main()
{
    // init clock
    CfgFsys();

    // Set LED_BIT as output, BTN_BITs as input
    P1_MOD_OC = 0b00100000;
    P1_DIR_PU = 0b10100000;

    // Set ROTA_BIT, ROTB_BIT as input
    P3_MOD_OC = 0b00011000;
    P3_DIR_PU = 0b00011000;

    // start usb client
    usb_init();

    // Start Timer0, Timer2
    TR0 = 1;
    TR2 = 1;

    // for debouncing the button
    bool button_state = false;

    while(1) {

        // read/debounce the button
        bool pressed = false;
        bool new_state = !BTN5_BIT;
        if(new_state != button_state && TF2 == 1) {
            TL2 = 0;
            TH2 = T2_DEBOUNCE;
            TF2 = 0;
            pressed = new_state;
            button_state = new_state;
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

        // queue up some keypresses if something happened
        if(pressed) {
            do_press(KEY_MEDIA_MUTE);
        }

        if(direction == -1) {
            do_press(KEY_MEDIA_VOL_UP);

        } else if(direction == 1) {
            do_press(KEY_MEDIA_VOL_DOWN);
        }

        // send key on/off to usb hid if there are some waiting to be sent
        if(usb_idle && !queue_empty()) {
            usb_set_keystate(queue_get());
        }
        led_update();

        // led flash admin
        // if(led_flash_time != 0) {
        //     if(TF0 == 1) {
        //         led_flash_time -= 1;
        //         TF0 = 0;
        //     }
        // } else {
        //     LED_BIT = 0;
        // }
    }
}