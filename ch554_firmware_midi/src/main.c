//////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ch554.h"
#include "ch554_usb.h"
#include "types.h"
#include "debug.h"
#include "util.h"
#include "usb.h"
#include "config.h"
#include "gpio.h"
#include "encoder.h"
#include "midi.h"
#include "main.h"

//////////////////////////////////////////////////////////////////////

// #define BOOTLOADER_DELAY 0x300    // about 3 seconds
#define BOOTLOADER_DELAY 0x80

#define DEBOUNCE_TIME 0xA0u
#define T2_DEBOUNCE (0xFFu - DEBOUNCE_TIME)

#define ROT_CLOCKWISE 2
#define ROT_ANTI_CLOCKWISE 0

// Define ROTARY_DIRECTION as ROT_CLOCKWISE for one kind of encoders, ROT_ANTI_CLOCKWISE for
// the other ones (some are reversed). This sets the default rotation (after first
// flash), reverse by triple-clicking the knob

#define ROTARY_DIRECTION (ROT_CLOCKWISE)
// #define ROTARY_DIRECTION (ROT_ANTI_CLOCKWISE)

//////////////////////////////////////////////////////////////////////
// XDATA, 1KB available

__xdata uint8 endpoint_0_buffer[DEFAULT_ENDP0_SIZE];     // endpoint0 OUT & IN buffer，Must be an even address
__xdata uint8 endpoint_1_buffer[DEFAULT_ENDP1_SIZE];     // endpoint1 upload buffer
__xdata uint8 endpoint_2_in_buffer[MAX_PACKET_SIZE];     // endpoint2 IN buffer, Must be an even address
__xdata uint8 endpoint_2_out_buffer[MAX_PACKET_SIZE];    // endpoint2 OUT buffer, Must be an even address
__xdata uint8 midi_send_buffer[48];
__xdata uint8 midi_recv_buffer[48];
__xdata save_buffer_t save_buffer;
__xdata uint8 queue_buffer[MIDI_QUEUE_LEN * MIDI_PACKET_SIZE];
__xdata config_t config;

uint8 button_value_index = 0;

//////////////////////////////////////////////////////////////////////
// led pulse/fade thing to indicate rotary/button activity

uint8 const led_gamma[256] = { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                               0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,
                               3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,   9,
                               9,   10,  10,  10,  11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,
                               21,  21,  22,  22,  23,  24,  24,  25,  25,  26,  27,  27,  28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  35,  36,  37,  38,
                               39,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
                               64,  66,  67,  68,  69,  70,  72,  73,  74,  75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,  90,  92,  93,  95,  96,  98,
                               99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
                               144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175, 177, 180, 182, 184, 186, 189, 191, 193, 196, 198,
                               200, 203, 205, 208, 210, 213, 215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255 };

uint16 led_brightness = 0;
bool led_idle = true;

#define LED_FADE_SPEED 3

//////////////////////////////////////////////////////////////////////

void led_update()
{
    if(led_brightness <= LED_FADE_SPEED) {
        LED_BIT = 0;
        led_idle = true;
    } else {
        led_brightness -= LED_FADE_SPEED;
        LED_BIT = led_gamma[led_brightness >> 7] > TH0;
    }

    if(led_idle && (config.flags & cf_led_track_button_toggle) != 0) {
        uint8 on_off = button_value_index;
        if((config.flags & cf_led_invert) != 0) {
            on_off = 1 - on_off;
        }
        LED_BIT = on_off;
    }
}

//////////////////////////////////////////////////////////////////////

inline void led_set_flash()
{
    led_idle = false;
    led_brightness = 32767u;
}

//////////////////////////////////////////////////////////////////////

void send_cc(uint8 channel, uint8 cc[2], uint16 value, bool is_extended)
{
    uint8 packet[MIDI_PACKET_SIZE];
    packet[0] = 0x0B;
    packet[1] = 0xB0 | channel;
    packet[2] = cc[0];
    if(is_extended) {
        packet[3] = (value >> 7) & 0x7F;
        queue_put(packet);
        packet[2] = cc[1];
        packet[3] = value & 0x7F;
    } else {
        packet[3] = value;
    }
    queue_put(packet);
}

//////////////////////////////////////////////////////////////////////

void send_button_cc()
{
    send_cc(get_btn_channel(), config.btn_control, config.btn_value[button_value_index], (config.flags & cf_btn_extended) != 0);
}

//////////////////////////////////////////////////////////////////////

void do_absolute_rotation(int16 offset)
{
    bool at_limit = false;
    int16 cur = config.rot_current_value;
    cur += offset;
    if(cur < (int16)config.rot_limit_low) {
        cur = config.rot_limit_low;
        at_limit = true;
    } else if(cur > (int16)config.rot_limit_high) {
        cur = config.rot_limit_high;
        at_limit = true;
    }
    config.rot_current_value = cur;
    send_cc(get_rot_channel(), config.rot_control, cur, (config.flags & cf_rotate_extended) != 0);

    if((config.flags & cf_led_flash_on_rot) != 0 || (at_limit && (config.flags & cf_led_flash_on_limit) != 0)) {
        led_set_flash();
    }
}

//////////////////////////////////////////////////////////////////////

void do_relative_rotation(int16 offset)
{
    int16 cur = config.rot_zero_point;
    cur += offset;
    if(cur < 0) {
        cur = 0;
    } else if(cur > 0x3fff) {
        cur = 0x3fff;
    }
    send_cc(get_rot_channel(), config.rot_control, cur, (config.flags & cf_rotate_extended) != 0);

    if((config.flags & cf_led_flash_on_rot) != 0) {
        led_set_flash();
    }
}

//////////////////////////////////////////////////////////////////////

int main()
{
    clk_init();
    delay_mS(5);
    uart0_init();

    putstr("================ BOOT =================\n");

    hexdump("default config", (uint8 *)&default_config, sizeof(default_config));

    init_chip_id();

    hexdump("CHIPID", &chip_id, 4);

    gpio_init(UART_TX_PORT, UART_TX_PIN, gpio_output_push_pull);
    gpio_init(UART_RX_PORT, UART_RX_PIN, gpio_output_open_drain);
    gpio_init(ROTA_PORT, ROTA_PIN, gpio_input_pullup);
    gpio_init(ROTB_PORT, ROTB_PIN, gpio_input_pullup);
    gpio_init(BTN_PORT, BTN_PIN, gpio_input_pullup);
    gpio_init(LED_PORT, LED_PIN, gpio_output_push_pull);

    load_config();

    TL2 = 0;
    TH2 = 0;
    TF2 = 0;

    TR0 = 1;
    TR2 = 1;

    led_flash(BOOT_FLASH_LED_COUNT, BOOT_FLASH_LED_SPEED);

    // init usb
    usb_device_config();
    usb_device_endpoint_config();    // Endpoint configuration
    usb_device_int_config();         // Interrupt initialization
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;    // Be pre -use and sending length must be empty
    UEP2_T_LEN = 0;    // Be pre -use and sending length must be empty


    uint16_t press_time = 0;
    bool button_state = false;    // for debouncing the button

    int8 turn_value = ROTARY_DIRECTION - 1;

    uint8 toggle = 0;

    while(1) {

        // read/debounce the button
        bool pressed = false;
        bool released = false;
        bool new_state = !BTN_BIT;

        if(new_state != button_state && TF2) {
            TL2 = 0;
            TH2 = T2_DEBOUNCE;
            TF2 = 0;
            pressed = new_state;
            released = !new_state;
            button_state = new_state;
        }

        if(button_state) {
            if(TF0 == 1) {
                TF0 = 0;
                press_time += 1;
                if(press_time == BOOTLOADER_DELAY) {

                    goto_bootloader();
                }
            }
        } else {
            press_time = 0;
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

        if(usb_config) {

            if(ep2_recv_len) {
                process_midi_packet_in(ep2_recv_len);
                UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
                ep2_recv_len = 0;
                handle_midi_packet();
            }

            // flush any waiting midi packets
            midi_flush_queue();

            // queue up any new waiting midi packets
            if(!midi_send_update()) {

                // no midi packets waiting to be sent, queue up any keypress/rotations
                if(!queue_full()) {

                    if(pressed) {

                        if(!is_toggle_mode()) {
                            button_value_index = 1;
                        } else {
                            config.flags ^= cf_toggle;
                            button_value_index = ((config.flags & cf_toggle) != 0) ? 1 : 0;
                        }
                        send_button_cc();
                        if((config.flags & cf_led_flash_on_click) != 0) {
                            led_set_flash();
                        }

                    } else if(released) {

                        if(!is_toggle_mode()) {
                            button_value_index = 0;
                            send_button_cc();
                        }
                        if((config.flags & cf_led_flash_on_release) != 0) {
                            led_set_flash();
                        }

                    } else if(direction != 0) {

                        int16 delta = config.rot_delta;
                        if(direction == turn_value) {
                            delta = -delta;
                        }

                        if((config.flags & cf_rotate_relative) != 0) {
                            do_relative_rotation(delta);
                        } else {
                            do_absolute_rotation(delta);
                        }
                    }
                }
            }
        }
        led_update();
    }
}
