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

#define DEBOUNCE_TIME 0x20u
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

// timer 0 irq sets this flag

volatile uint8 tick;

uint8 button_value_index = 0;

//////////////////////////////////////////////////////////////////////
// led pulse/fade thing to indicate rotary/button activity

uint8 const led_gamma[256] = {
    0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,   0u,
    0u,   0u,   1u,   1u,   1u,   1u,   1u,   1u,   1u,   1u,   1u,   1u,   1u,   1u,   1u,   2u,   2u,   2u,   2u,   2u,   2u,   2u,   2u,   3u,   3u,   3u,
    3u,   3u,   3u,   3u,   4u,   4u,   4u,   4u,   4u,   5u,   5u,   5u,   5u,   6u,   6u,   6u,   6u,   7u,   7u,   7u,   7u,   8u,   8u,   8u,   9u,   9u,
    9u,   10u,  10u,  10u,  11u,  11u,  11u,  12u,  12u,  13u,  13u,  13u,  14u,  14u,  15u,  15u,  16u,  16u,  17u,  17u,  18u,  18u,  19u,  19u,  20u,  20u,
    21u,  21u,  22u,  22u,  23u,  24u,  24u,  25u,  25u,  26u,  27u,  27u,  28u,  29u,  29u,  30u,  31u,  32u,  32u,  33u,  34u,  35u,  35u,  36u,  37u,  38u,
    39u,  39u,  40u,  41u,  42u,  43u,  44u,  45u,  46u,  47u,  48u,  49u,  50u,  50u,  51u,  52u,  54u,  55u,  56u,  57u,  58u,  59u,  60u,  61u,  62u,  63u,
    64u,  66u,  67u,  68u,  69u,  70u,  72u,  73u,  74u,  75u,  77u,  78u,  79u,  81u,  82u,  83u,  85u,  86u,  87u,  89u,  90u,  92u,  93u,  95u,  96u,  98u,
    99u,  101u, 102u, 104u, 105u, 107u, 109u, 110u, 112u, 114u, 115u, 117u, 119u, 120u, 122u, 124u, 126u, 127u, 129u, 131u, 133u, 135u, 137u, 138u, 140u, 142u,
    144u, 146u, 148u, 150u, 152u, 154u, 156u, 158u, 160u, 162u, 164u, 167u, 169u, 171u, 173u, 175u, 177u, 180u, 182u, 184u, 186u, 189u, 191u, 193u, 196u, 198u,
    200u, 203u, 205u, 208u, 210u, 213u, 215u, 218u, 220u, 223u, 225u, 228u, 231u, 233u, 236u, 239u, 241u, 244u, 247u, 249u, 252u, 255u
};

uint16 led_brightness = 0;

#define LED_FADE_SPEED 4

//////////////////////////////////////////////////////////////////////

void led_update()
{
    if(led_brightness <= LED_FADE_SPEED) {
        LED_BIT = 0;
        led_brightness = 0;
    } else {
        led_brightness -= LED_FADE_SPEED;
        LED_BIT = led_gamma[led_brightness >> 8] > TL2;
    }

    if(led_brightness == 0 && (config.flags & cf_led_track_button_toggle) != 0) {
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
    led_brightness = 65535u;
}

//////////////////////////////////////////////////////////////////////

void timer0_irq_handler(void) __interrupt(INT_NO_TMR0)
{
    TL0 = TIMER0_LOW;
    TH0 = TIMER0_HIGH;
    tick = 1;
}

//////////////////////////////////////////////////////////////////////
// Send a control change notification
//
//  if is_extended
//      send 14 bit value as two midi packets
//  else
//      send 7 bit value as a single midi packet

void send_cc(uint8 channel, uint8 cc[2], uint16 value, bool is_extended)
{
    uint8 packet[MIDI_PACKET_SIZE];

    // CC header
    packet[0] = 0x0B;
    packet[1] = 0xB0 | channel;

    // cc[0] is the value to send (or MSB of value)
    packet[2] = cc[0];

    // send MSB first if extended mode
    if(is_extended) {

        // send MSB of value
        packet[3] = (value >> 7) & 0x7F;
        queue_put(packet);

        // cc[1] is LSB of value
        packet[2] = cc[1];
    }

    // send value (or LSB of value)
    packet[3] = value & 0x7f;
    queue_put(packet);
}

//////////////////////////////////////////////////////////////////////

void send_button_cc()
{
    send_cc(get_btn_channel(), config.btn_control, config.btn_value[button_value_index], (config.flags & cf_btn_extended) != 0);
}

//////////////////////////////////////////////////////////////////////

int16 rotation_velocity = 0;
uint8 deceleration_ticks = 0;

void do_absolute_rotation(int16 offset)
{
    bool at_limit = false;
    int32 cur = config.rot_current_value;
    cur += (int32)offset * (rotation_velocity + 1);
    if(cur < (int32)config.rot_limit_low) {
        cur = config.rot_limit_low;
        at_limit = true;
    } else if(cur > (int32)config.rot_limit_high) {
        cur = config.rot_limit_high;
        at_limit = true;
    }
    config.rot_current_value = (int16)cur;
    send_cc(get_rot_channel(), config.rot_control, cur, (config.flags & cf_rotate_extended) != 0);

    if((config.flags & cf_led_flash_on_rot) != 0 || (at_limit && (config.flags & cf_led_flash_on_limit) != 0)) {
        led_set_flash();
    }
}

//////////////////////////////////////////////////////////////////////

void do_relative_rotation(int16 offset)
{
    int32 cur = config.rot_zero_point;
    cur += (int32)offset * (rotation_velocity + 1);
    int16 cur_limit = 0x7f;
    bool extended = (config.flags & cf_rotate_extended) != 0;
    if(extended) {
        cur_limit = 0x3fff;
    }
    if(cur < 0) {
        cur = 0;
    } else if(cur > cur_limit) {
        cur = cur_limit;
    }
    send_cc(get_rot_channel(), config.rot_control, (int16)cur, extended);

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

    // init timer2 - all default values
    TL2 = 0;
    TH2 = 0;
    TF2 = 0;

    // timer0 16 bit mode
    TMOD = (TMOD & ~MASK_T0_MOD) | 0x01;

    TL0 = TIMER0_LOW;
    TH0 = TIMER0_HIGH;

    // clear overflow flags
    TF0 = 0;

    // enable irq
    ET0 = 1;
    EA = 1;

    // enable the timers
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

        if(tick) {
            deceleration_ticks += 1;
            tick = 0;
            if(button_state) {
                press_time += 1;
                if(press_time == BOOTLOADER_BUTTON_DELAY) {

                    goto_bootloader();
                }
            } else {
                press_time = 0;
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

                        int16 limit = (config.flags & cf_rotate_extended) ? 0x3fff : 0x7f;

                        rotation_velocity += get_acceleration();

                        if(rotation_velocity >= limit) {
                            rotation_velocity = limit;
                        }

                        deceleration_ticks = 0;

                    } else {
                        if(deceleration_ticks == 50) {
                            deceleration_ticks = 0;
                            rotation_velocity >>= 1;
                        }
                    }
                }
            }
        }
        led_update();
    }
}
