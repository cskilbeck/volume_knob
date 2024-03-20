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

enum hid_custom_command
{
    // send the config to the web ui
    hcc_get_config = 1,

    // send the firmware version to the web ui
    hcc_get_firmware_version = 2,

    // flash the led please
    hcc_flash_led = 3,

    // here's a new config
    hcc_set_config = 4,

    // reboot into firmware flashing mode
    hcc_goto_bootloader = 5
};

//////////////////////////////////////////////////////////////////////

enum hid_custom_response
{
    hcc_here_is_config = 1,
    hcc_here_is_firmware_version = 2,
    hcc_led_flashed = 3,
    hcc_set_config_ack = 4
};

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

//////////////////////////////////////////////////////////////////////
// check it's not empty before calling these

void queue_pop_front()
{
    queue_size -= 1;
    queue_head = ++queue_head & (KEY_QUEUE_LEN - 1);
}

//////////////////////////////////////////////////////////////////////

uint16 queue_get()
{
    uint16 next = queue_buffer[queue_head];
    queue_pop_front();
    return next;
}

//////////////////////////////////////////////////////////////////////

uint16 queue_peek()
{
    return queue_buffer[queue_head];
}

//////////////////////////////////////////////////////////////////////
// top bit specifies whether it's a media key (1) or normal (0)

bool set_keystate(uint16 key)
{
    if(IS_MEDIA_KEY(key) && usb_is_endpoint_idle(endpoint_2)) {
        key &= 0x7fff;
        usb_endpoint_2_tx_buffer[0] = 0x02;    // REPORT ID
        usb_endpoint_2_tx_buffer[1] = key & 0xff;
        usb_endpoint_2_tx_buffer[2] = key >> 8;
        usb_send(endpoint_2, 3);
        return true;
    }
    if(!IS_MEDIA_KEY(key) && usb_is_endpoint_idle(endpoint_1)) {
        usb_endpoint_1_tx_buffer[0] = 0x00;    // keyboard modifier
        usb_endpoint_1_tx_buffer[1] = 0x00;
        usb_endpoint_1_tx_buffer[2] = key;    // keyboard key
        usb_endpoint_1_tx_buffer[3] = 0x00;
        usb_endpoint_1_tx_buffer[4] = 0x00;
        usb_endpoint_1_tx_buffer[5] = 0x00;
        usb_endpoint_1_tx_buffer[6] = 0x00;
        usb_endpoint_1_tx_buffer[7] = 0x00;
        usb_send(endpoint_1, 8);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
// Received something from the Web UI page

void handle_custom_hid_packet()
{
    printf("Got HID cmd: %d\n", usb_endpoint_3_rx_buffer[0]);

    switch(usb_endpoint_3_rx_buffer[0]) {

    case hcc_get_config:
        if(usb_is_endpoint_idle(endpoint_3)) {
            usb_endpoint_3_tx_buffer[0] = hcc_here_is_config;
            memcpy(usb_endpoint_3_tx_buffer + 1, &config, sizeof(config));
            usb_send(endpoint_3, 32);
        }
        break;

    case hcc_set_config:
        memcpy(&config, usb_endpoint_3_rx_buffer + 1, sizeof(config));
        save_config();
        usb_endpoint_3_tx_buffer[0] = hcc_set_config_ack;
        usb_send(endpoint_3, 32);
        break;

    case hcc_get_firmware_version:
        if(usb_is_endpoint_idle(endpoint_3)) {
            usb_endpoint_3_tx_buffer[0] = hcc_here_is_firmware_version;
            usb_endpoint_3_tx_buffer[1] = (uint8)(FIRMWARE_VERSION >> 0);
            usb_endpoint_3_tx_buffer[2] = (uint8)(FIRMWARE_VERSION >> 8);
            usb_endpoint_3_tx_buffer[3] = (uint8)(FIRMWARE_VERSION >> 16);
            usb_endpoint_3_tx_buffer[4] = (uint8)(FIRMWARE_VERSION >> 24);
            usb_send(endpoint_3, 32);
        } else {
            puts("3 busy!");
        }
        break;

    case hcc_flash_led:
        led_flash();
        puts("flashed");
        if(usb_is_endpoint_idle(endpoint_3)) {
            puts("replied");
            usb_endpoint_3_tx_buffer[0] = hcc_led_flashed;
            usb_send(endpoint_3, 32);
        } else {
            printf("%d", usb.idle);
        }
        break;

    case hcc_goto_bootloader:
        goto_bootloader();
        break;
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

    load_config();

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
        bool released = false;
        bool new_state = !BTN_BIT;

        if(new_state != button_state && button_ticks > 1) {

            button_ticks = 0;
            pressed = new_state;
            released = !new_state;
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
                    config.flags ^= cf_reverse_rotation;
                    save_config();
                    pressed = false;
                }
            }
            button_click_tick_count = 0;
        }

        if(usb.recv_len[3] != 0) {
            handle_custom_hid_packet();
            usb.recv_len[3] = 0;
        }

        // queue up some keypresses if something happened
        if(pressed) {

            if((config.flags & cf_led_flash_on_press) != 0) {
                led_flash();
            }

            uint8 space = 1;
            bool send_release = config.key_release == 0;
            if(send_release) {
                space = 2;
            }

            if(queue_space() >= space) {
                queue_put(config.key_press);
                if(send_release) {
                    queue_put(config.key_press & 0x8000);
                }
            }
        }

        if(released) {
            if((config.flags & cf_led_flash_on_release) != 0) {
                led_flash();
            }

            if(config.key_release != 0 && queue_space() >= 1) {
                queue_put(config.key_release);
            }
        }

        int8 turn_value = ((config.flags & cf_reverse_rotation) != 0) ? -1 : 1;

        if(direction == turn_value) {

            if((config.flags & cf_led_flash_on_cw) != 0) {
                led_flash();
            }

            if(queue_space() >= 2) {
                queue_put(config.key_clockwise);
                queue_put(config.key_clockwise & 0x8000);
            }

        } else if(direction == -turn_value) {

            if((config.flags & cf_led_flash_on_ccw) != 0) {
                led_flash();
            }

            if(queue_space() >= 2) {
                queue_put(config.key_counterclockwise);
                queue_put(config.key_counterclockwise & 0x8000);
            }
        }

        // send key on/off to usb hid if there are some waiting to be sent
        if(!queue_empty() && set_keystate(queue_peek())) {
            queue_pop_front();
        }
        led_update();
    }
}
