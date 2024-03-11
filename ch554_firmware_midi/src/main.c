//////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ch554.h"
#include "ch554_usb.h"
#include "types.h"
#include "gpio.h"
#include "debug.h"
#include "util.h"
#include "usb.h"
#include "config.h"
#include "encoder.h"
#include "midi.h"
#include "main.h"

//////////////////////////////////////////////////////////////////////

#define DEBOUNCE_TIME 0x1Cu
#define T2_DEBOUNCE (0xFFu - DEBOUNCE_TIME)

#define ROT_CLOCKWISE 2
#define ROT_ANTI_CLOCKWISE 0

// Define ROTARY_DIRECTION as ROT_CLOCKWISE for one kind of encoders, ROT_ANTI_CLOCKWISE for
// the other ones (some are reversed). This sets the default rotation (after first
// flash), reverse by triple-clicking the knob

#if DEVICE == DEVICE_DEVKIT

#define ROTARY_DIRECTION (ROT_CLOCKWISE)
#define LED_ON 1
#define LED_OFF 0

#else

#define ROTARY_DIRECTION (ROT_ANTI_CLOCKWISE)
#define LED_OFF 1
#define LED_ON 0

#endif

//////////////////////////////////////////////////////////////////////
// XDATA, 1KB available

__xdata uint8 endpoint_0_buffer[DEFAULT_ENDP0_SIZE];     // endpoint0 OUT & IN buffer，Must be an even address
__xdata uint8 endpoint_1_buffer[DEFAULT_ENDP1_SIZE];     // endpoint1 upload buffer
__xdata uint8 endpoint_2_in_buffer[MAX_PACKET_SIZE];     // endpoint2 IN buffer, Must be an even address
__xdata uint8 endpoint_2_out_buffer[MAX_PACKET_SIZE];    // endpoint2 OUT buffer, Must be an even address
__xdata uint8 serial_number_string[18];
__xdata uint8 midi_send_buffer[48];
__xdata uint8 midi_recv_buffer[48];
__xdata save_buffer_t save_buffer;
__xdata uint8 queue_buffer[MIDI_QUEUE_LEN * MIDI_PACKET_SIZE];
__xdata config_t config;

// timer 0 irq sets this flag

volatile uint8 tick;

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
        LED_BIT = LED_OFF;
        led_brightness = 0;
    } else {
        led_brightness -= LED_FADE_SPEED;
        LED_BIT = (led_gamma[led_brightness >> 8] > TL2) ? LED_ON : LED_OFF;
    }

    if(led_brightness == 0 && config_flag(cf_led_track_button_toggle)) {
        uint8 on_off = config_flag(cf_toggle);
        if(config_flag(cf_led_invert)) {
            on_off = 1 - on_off;
        }
        LED_BIT = on_off ? LED_ON : LED_OFF;
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

void send_cc(uint8 channel, uint8 cc_msb, uint8 cc_lsb, uint16 value, bool is_extended)
{
    uint8 packet[MIDI_PACKET_SIZE];

    // CC header
    packet[0] = 0x0B;
    packet[1] = 0xB0 | channel;

    // cc[0] is the value to send (or MSB of value)
    packet[2] = cc_msb;

    // send MSB first if extended mode
    if(is_extended) {

        // send MSB of value
        packet[3] = (value >> 7) & 0x7F;
        queue_put(packet);

        // cc[1] is LSB of value
        packet[2] = cc_lsb;
    }

    // send value (or LSB of value)
    packet[3] = value & 0x7f;
    queue_put(packet);
}

//////////////////////////////////////////////////////////////////////

typedef enum
{
    value_a = 0,
    value_b = 1
} which_value_t;

void send_button_cc(which_value_t val)
{
    bool extended = config_flag(cf_btn_extended);

    uint16 value = extended ? config.btn_value_a_14 : config.btn_value_a_7;

    if(val == value_b) {

        value = extended ? config.btn_value_b_14 : config.btn_value_b_7;
    }
    send_cc(get_btn_channel(), config.btn_control_msb, config.btn_control_lsb, value, extended);
}

//////////////////////////////////////////////////////////////////////

int16 rotation_velocity = 0;
uint8 deceleration_ticks = 0;

void do_absolute_rotation(int16 offset)
{
    bool extended = config_flag(cf_rotate_extended);
    bool at_limit = false;
    int32 cur = extended ? config.rot_current_value_14 : config.rot_current_value_7;
    int16 limit = extended ? 0x3fff : 0x7f;
    cur += (int32)offset * (rotation_velocity + 1);
    if(cur < 0) {
        cur = 0;
        at_limit = true;
    } else if(cur > limit) {
        cur = limit;
        at_limit = true;
    }
    if(extended) {
        config.rot_current_value_14 = (int16)cur;
    } else {
        config.rot_current_value_7 = (int16)cur;
    }
    send_cc(get_rot_channel(), config.rot_control_msb, config.rot_control_lsb, cur, extended);

    if(config_flag(cf_led_flash_on_rot) || (at_limit && config_flag(cf_led_flash_on_limit))) {

        led_set_flash();
    }
}

//////////////////////////////////////////////////////////////////////
// relative rotation is forced 7 bit mode

void do_relative_rotation(int16 offset)
{
    uint8 cur = (config.rot_zero_point + offset * (rotation_velocity + 1)) & 0x7f;
    send_cc(get_rot_channel(), config.rot_control_msb, 0, cur, false);

    if(config_flag(cf_led_flash_on_rot)) {
        led_set_flash();
    }
}

//////////////////////////////////////////////////////////////////////

int main()
{
    clk_init();
    delay_mS(5);

#if DEVICE == DEVICE_DEVKIT
    gpio_init(UART_TX_PORT, UART_TX_PIN, gpio_output_push_pull);
    gpio_init(UART_RX_PORT, UART_RX_PIN, gpio_output_open_drain);
    uart0_init();
#endif

    putstr("================ BOOT =================\n");

    hexdump("default config", (uint8 *)&default_config, sizeof(default_config));

    init_chip_id();

    hexdump("CHIPID", &chip_id, 4);

    // setup USB serial string from Chip ID

    serial_number_string[0] = 18;    // 18 long, 2 for header, 16 for the serial number (8 wchars)
    serial_number_string[1] = 3;     // usb string descriptor
    uint32 n = chip_id;
    for(uint8 i = 0; i < 8; ++i) {
        uint8 c = (uint8)(n >> 28);
        if(c < 10) {
            c += '0';
        } else {
            c += 'A' - 10;
        }
        serial_number_string[i * 2 + 2] = c;
        serial_number_string[i * 2 + 3] = 0;
        n <<= 4;
    }

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

                    // BUTTON

                    if(pressed) {

                        config.flags ^= cf_toggle;

                        which_value_t value = value_a;
                        if(!config_flag(cf_btn_momentary) && config_flag(cf_toggle)) {
                            value = value_b;
                        }
                        send_button_cc(value);

                        if(config_flag(cf_led_flash_on_click)) {
                            led_set_flash();
                        }

                    } else if(released) {

                        if(config_flag(cf_btn_momentary)) {
                            send_button_cc(value_b);
                        }

                        if(config_flag(cf_led_flash_on_release)) {
                            led_set_flash();
                        }
                    }

                    // ROTATION

                    if(direction != 0) {

                        if(config_flag(cf_rotate_reverse)) {
                            direction = -direction;
                        }

                        int16 delta = config_flag(cf_rotate_extended) ? config.rot_delta_14 : config.rot_delta_7;
                        if(direction == turn_value) {
                            delta = -delta;
                        }

                        if(config_flag(cf_rotate_relative)) {
                            do_relative_rotation(delta);
                        } else {
                            do_absolute_rotation(delta);
                        }

                        int16 limit = config_flag(cf_rotate_extended) ? 0x3fff : 0x7f;

                        rotation_velocity += get_acceleration();

                        if(rotation_velocity >= limit) {
                            rotation_velocity = limit;
                        }

                        deceleration_ticks = 0;

                    } else if(deceleration_ticks == 50) {

                        deceleration_ticks = 0;
                        rotation_velocity >>= 1;
                    }
                }
            }
        }
        led_update();
    }
}
