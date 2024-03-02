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

//////////////////////////////////////////////////////////////////////

// #define BOOTLOADER_DELAY 0x300    // about 3 seconds
#define BOOTLOADER_DELAY 0x80

#define DEBOUNCE_TIME 0xA0u
#define T2_DEBOUNCE (0xFFu - DEBOUNCE_TIME)

#define BOOT_FLASH_LED_COUNT 10
#define BOOT_FLASH_LED_SPEED 0x40

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

void do_absolute_rotation(int16 offset)
{
    int16 cur = config.rot_current_value;
    cur += offset;
    if(cur < (int16)config.rot_limit_low) {
        cur = config.rot_limit_low;
    } else if(cur > (int16)config.rot_limit_high) {
        cur = config.rot_limit_high;
    }
    config.rot_current_value = cur;
    send_cc(get_rot_channel(), config.rot_control, cur, (config.flags & cf_rotate_extended) != 0);
}

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
}

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

                        uint8 index = 0;
                        if(is_toggle_mode()) {
                            config.flags ^= cf_toggle;
                            if((config.flags & cf_toggle) != 0) {
                                index = 1;
                            }
                        }

                        send_cc(get_btn_channel(), config.btn_control, config.btn_value[index], (config.flags & cf_btn_extended) != 0);
                        LED_BIT = !LED_BIT;

                    } else if(released) {

                        if(!is_toggle_mode()) {
                            send_cc(get_btn_channel(), config.btn_control, config.btn_value[1], (config.flags & cf_btn_extended) != 0);
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
                        LED_BIT = !LED_BIT;
                    }
                }
            }
        }
    }
}
