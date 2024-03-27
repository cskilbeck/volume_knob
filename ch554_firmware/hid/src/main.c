#include "main.h"

#define XDATA __xdata
#include "xdata_extra.h"
#undef XDATA

//////////////////////////////////////////////////////////////////////
// TIMER0 = LED pwm
// TIMER1 = UART
// TIMER2 = 1KHz tick

//////////////////////////////////////////////////////////////////////

// time between button clicks to be considered a double click
#define BUTTON_QUICK_CLICK_MS 333

// how long to hold it for a long click
#define BUTTON_LONG_CLICK_MS 1000

//////////////////////////////////////////////////////////////////////
// a queue of events

typedef enum event
{
    event_clockwise = 1,
    event_counterclockwise = 2,
    event_press = 3,

    event_mask = 0x3f,

    event_keydown = 0,     // key down is on by default
    event_keyup = 0x80,    // else key up

} event_t;

//////////////////////////////////////////////////////////////////////

void hid_process_event(event_t e)
{
    uint8 modifiers = 0;
    uint16 key = 0;

    switch(e & event_mask) {

    case event_clockwise:
        key = config.key_clockwise;
        modifiers = config.mod_clockwise;
        break;

    case event_counterclockwise:
        key = config.key_counterclockwise;
        modifiers = config.mod_counterclockwise;
        break;

    case event_press:
        key = config.key_press;
        modifiers = config.mod_press;
        break;
    }

    uint16 send_key = key & 0x7fff;
    uint16 send_modifiers = modifiers;

    if((e & event_keyup) != 0) {
        send_key = 0;
        send_modifiers = 0;
    }

    if(IS_MEDIA_KEY(key)) {

        consumer_control_hid_report *cc = (consumer_control_hid_report *)usb_endpoint_1_tx_buffer;
        cc->report_id = 0x02;
        cc->keycode = send_key;
        usb_send(endpoint_1, 3);

    } else {

        keyboard_hid_report *k = ((keyboard_hid_report *)usb_endpoint_1_tx_buffer);
        k->report_id = 0x01;
        k->modifiers = send_modifiers;
        k->pad = 0;
        k->key[0] = send_key;
        k->key[1] = 0;
        k->key[2] = 0;
        k->key[3] = 0;
        k->key[4] = 0;
        k->key[5] = 0;
        usb_send(endpoint_1, 9);
    }
}

//////////////////////////////////////////////////////////////////////
// Received something from the Web UI page

void handle_custom_hid_packet()
{
    printf("Custom HID: %d\n", usb_endpoint_3_rx_buffer[0]);

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
        usb_endpoint_3_tx_buffer[0] = hcc_set_config_ack;
        usb_endpoint_3_tx_buffer[1] = 0;
        if(save_config()) {
            usb_endpoint_3_tx_buffer[1] = 1;
        }
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
        }
        break;

    case hcc_flash_led:
        led_flash();
        if(usb_is_endpoint_idle(endpoint_3)) {
            usb_endpoint_3_tx_buffer[0] = hcc_led_flashed;
            usb_send(endpoint_3, 32);
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

    // if button held down when the device is plugged in, go to bootloader
    if(BTN_BIT == 0) {
        goto_bootloader();
    }

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

    // Double click admin

    uint16 button_click_tick_count = 0;
    uint8 button_quick_clicks = 0;

    bool button_state = false;
    uint8 button_debounce_ticks = 0;    // for debouncing the button

    // main loop

    puts("Main loop");

    QUEUE_INIT(hid_queue);

    while(1) {

        // read/debounce the button
        bool pressed = false;
        bool released = false;
        bool new_state = !BTN_BIT;

        if(new_state != button_state && button_debounce_ticks > 1) {

            button_debounce_ticks = 0;
            pressed = new_state;
            released = !new_state;
            button_state = new_state;
        }

        // if tick
        if(TF2) {

            TF2 = 0;

            if(button_debounce_ticks <= 2) {
                button_debounce_ticks += 1;
            }

            if(button_state) {
                press_time += 1;
                if(press_time == BUTTON_LONG_CLICK_MS) {
                    puts("Long click");
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

        // check for double-click
        if(pressed) {
            if(button_click_tick_count < BUTTON_QUICK_CLICK_MS) {
                if(button_quick_clicks == 2) {
                    puts("Double Click");
                    pressed = false;
                    button_quick_clicks = 0;
                } else {
                    button_quick_clicks += 1;
                }
            }
            button_click_tick_count = 0;
        }

        // if something arrived on endpoint 3
        if(usb.recv_len[3] != 0) {

            // handle it
            handle_custom_hid_packet();
            usb.recv_len[3] = 0;
        }

        // queue up some events if something happened
        if(pressed) {

            if((config.flags & cf_led_flash_on_press) != 0) {
                led_flash();
            }

            if(QUEUE_SPACE(hid_queue) >= 1) {
                QUEUE_PUSH(hid_queue, event_press | event_keydown);
            }
        }

        if(released) {

            if((config.flags & cf_led_flash_on_release) != 0) {
                led_flash();
            }

            if(QUEUE_SPACE(hid_queue) >= 1) {
                QUEUE_PUSH(hid_queue, event_press | event_keyup);
            }
        }

        int8 turn_value = ((config.flags & cf_reverse_rotation) != 0) ? 1 : -1;

        if(direction == turn_value) {

            if((config.flags & cf_led_flash_on_cw) != 0) {
                led_flash();
            }

            if(QUEUE_SPACE(hid_queue) >= 2) {
                QUEUE_PUSH(hid_queue, event_clockwise | event_keydown);
                QUEUE_PUSH(hid_queue, event_clockwise | event_keyup);
            }

        } else if(direction == -turn_value) {

            if((config.flags & cf_led_flash_on_ccw) != 0) {
                led_flash();
            }

            if(QUEUE_SPACE(hid_queue) >= 2) {
                QUEUE_PUSH(hid_queue, event_counterclockwise | event_keydown);
                QUEUE_PUSH(hid_queue, event_counterclockwise | event_keyup);
            }
        }

        // send key on/off to usb hid if there are some waiting to be sent
        if(!QUEUE_IS_EMPTY(hid_queue) && usb_is_endpoint_idle(endpoint_1)) {
            uint8 p;
            QUEUE_POP(hid_queue, p);
            hid_process_event(p);
        }
        led_update();
    }
}
