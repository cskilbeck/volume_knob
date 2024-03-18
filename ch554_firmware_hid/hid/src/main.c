#include "main.h"

//////////////////////////////////////////////////////////////////////
// TIMER0 = LED pwm
// TIMER1 = UART
// TIMER2 = 1KHz tick

//////////////////////////////////////////////////////////////////////
// Define ROTARY_DIRECTION as CLOCKWISE for one kind of encoders, ANTI_CLOCKWISE for
// the other ones (some are reversed). This sets the default rotation (after first
// flash), reverse by triple-clicking the knob

#define CLOCKWISE 2
#define ANTI_CLOCKWISE 0

#if DEVICE == DEVICE_DEVKIT
#define ROTARY_DIRECTION (ANTI_CLOCKWISE)
#else
#define ROTARY_DIRECTION (CLOCKWISE)
#endif

//////////////////////////////////////////////////////////////////////

#define MEDIA_KEY(x) ((x) | 0x8000)
#define NORMAL_KEY(x) (x)

//////////////////////////////////////////////////////////////////////

// time between button clicks to be considered quick clicks
#define BUTTON_QUICK_CLICK_MS 500

// # of quick clicks to reverse direction
#define BUTTON_QUICK_CLICK_COUNT 3

//////////////////////////////////////////////////////////////////////
// a queue of key states to send

// must be a power of 2
#define KEY_QUEUE_LEN 16

__idata uint16 queue_buffer[KEY_QUEUE_LEN];
__idata uint8 queue_head = 0;
__idata uint8 queue_size = 0;

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
    if(queue_space() >= 2) {
        queue_put(k);
        queue_put(k & 0x8000);
        led_flash();
    }
}

//////////////////////////////////////////////////////////////////////
// top bit specifies whether it's a media key (1) or normal (0)

#define IS_MEDIA_KEY(x) ((x & 0x8000) != 0)

void set_keystate(uint16 key)
{
    if(!IS_MEDIA_KEY(key)) {
        usb_endpoint_1_buffer[0] = 0x00;    // keyboard modifier
        usb_endpoint_1_buffer[1] = 0x00;
        usb_endpoint_1_buffer[2] = key;    // keyboard key
        usb_endpoint_1_buffer[3] = 0x00;
        usb_endpoint_1_buffer[4] = 0x00;
        usb_endpoint_1_buffer[5] = 0x00;
        usb_endpoint_1_buffer[6] = 0x00;
        usb_endpoint_1_buffer[7] = 0x00;
        usb_idle &= ~1;
        UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
        UEP1_T_LEN = 8;
    } else {
        key &= 0x7fff;
        usb_endpoint_2_buffer[0] = 0x02;    // REPORT ID
        usb_endpoint_2_buffer[1] = key & 0xff;
        usb_endpoint_2_buffer[2] = key >> 8;
        usb_idle &= ~2;
        UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
        UEP2_T_LEN = 3;
    }
}

//////////////////////////////////////////////////////////////////////

#define CLEAR_CONSOLE "\033c\033[3J\033[2J"

void main()
{
    clk_init();
    tick_init();
    uart0_init();
    led_init();
    encoder_init();

    // global irq enable
    EA = 1;

    printf(CLEAR_CONSOLE "---------- HID ----------\nCHIP ID: %08lx\n", chip_id);

    uint8 vol_direction;
    int8 turn_value;

    read_flash_data(0, 1, &vol_direction);

    switch(vol_direction) {
    case 0:
        turn_value = -1;
        break;
    case 2:
        turn_value = 1;
        break;
    default:
        vol_direction = ROTARY_DIRECTION;
        turn_value = ROTARY_DIRECTION - 1;
        break;
    }
    printf("TURN: %d\n", turn_value);

    usb_init_strings();
    usb_device_config();
    usb_device_endpoint_config();
    usb_device_int_config();

    led_flash_n_times(BOOT_FLASH_LED_COUNT, BOOT_FLASH_LED_SPEED);

    usb_wait_for_connection();

    uint16 press_time = 0;

    // Triple click admin

    uint16 button_click_tick_count = 0;
    uint8 button_quick_clicks = 0;

    bool button_state = false;
    uint8 button_ticks = 0;    // for debouncing the button

    // main loop

    puts("Main loop");

    while(1) {

        // read/debounce the button
        bool pressed = false;
        bool new_state = !BTN_BIT;

        if(new_state != button_state && button_ticks > 1) {

            button_ticks = 0;
            pressed = new_state;
            button_state = new_state;
        }

        // if tick
        if(TF2) {

            TF2 = 0;

            if(button_ticks <= 2) {
                button_ticks += 1;
            }

            if(button_state) {
                press_time += 1;
                if(press_time == BOOTLOADER_BUTTON_DELAY_MS) {
                    goto_bootloader();
                }
            } else {
                press_time = 0;
            }

            // delay counter for knob triple-click
            if(button_click_tick_count < BUTTON_QUICK_CLICK_MS) {
                button_click_tick_count += 1;
            } else {
                button_quick_clicks = 0;
            }

            // led flash fade
            led_on_tick();
        }

        // read the rotary encoder (returns -1, 0 or 1)
        int8 direction = encoder_read();

        // check for triple-click
        if(pressed) {
            if(button_click_tick_count < BUTTON_QUICK_CLICK_MS) {
                button_quick_clicks += 1;
                if(button_quick_clicks == (BUTTON_QUICK_CLICK_COUNT - 1)) {
                    vol_direction = 2 - vol_direction;
                    turn_value = (int8)vol_direction - 1;
                    printf("NEW TURN: %d\n", turn_value);
                    write_flash_data(0, 1, &vol_direction);
                    pressed = false;
                }
            }
            button_click_tick_count = 0;
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
        if((usb_idle & 3) == 3 && !queue_empty()) {

            set_keystate(queue_get());
        }
        led_update();
    }
}
