//////////////////////////////////////////////////////////////////////

#include "main.h"

//////////////////////////////////////////////////////////////////////
// TIMER0 = LED pwm
// TIMER1 = UART
// TIMER2 = 1KHz tick

//////////////////////////////////////////////////////////////////////

#define ROT_CLOCKWISE 2
#define ROT_ANTI_CLOCKWISE 0

// Define ROTARY_DIRECTION as ROT_CLOCKWISE for one kind of encoders, ROT_ANTI_CLOCKWISE for
// the other ones (some are reversed). This sets the default rotation (after first
// flash), reverse by triple-clicking the knob

#if DEVICE == DEVICE_DEVKIT

#define ROTARY_DIRECTION (ROT_CLOCKWISE)
#else
#define ROTARY_DIRECTION (ROT_ANTI_CLOCKWISE)
#endif

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

static int16 rotation_velocity = 0;
static uint8 deceleration_ticks = 0;

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

        led_flash();
    }
}

//////////////////////////////////////////////////////////////////////
// relative rotation is forced 7 bit mode

void do_relative_rotation(int16 offset)
{
    uint8 cur = (config.rot_zero_point + offset * (rotation_velocity + 1)) & 0x7f;
    send_cc(get_rot_channel(), config.rot_control_msb, 0, cur, false);

    if(config_flag(cf_led_flash_on_rot)) {
        led_flash();
    }
}

//////////////////////////////////////////////////////////////////////

#define CLEAR_CONSOLE "\033c\033[3J\033[2J"

int main()
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

    printf(CLEAR_CONSOLE "---------- MIDI ----------\nCHIP ID: %08lx\n", chip_id);

    hexdump("default config", (uint8 *)&default_config, sizeof(default_config));

    load_config();

    usb_init_strings();
    usb_device_config();
    usb_device_endpoint_config();
    usb_device_int_config();

    led_flash_n_times(BOOT_FLASH_LED_COUNT, BOOT_FLASH_LED_SPEED);

    usb_wait_for_connection();

    uint16 press_time = 0;
    uint8 button_ticks = 0;
    bool button_state = false;    // for debouncing the button

    int8 turn_value = ROTARY_DIRECTION - 1;

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
            deceleration_ticks += 1;
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
            led_on_tick();
        }

        // read the rotary encoder (returns -1, 0 or 1)
        int8 direction = encoder_read();

        if(usb.recv_len[2] != 0) {
            process_midi_packet_in(usb.recv_len[2]);
            UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
            usb.recv_len[2] = 0;
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
                        led_flash();
                    }

                } else if(released) {

                    if(config_flag(cf_btn_momentary)) {
                        send_button_cc(value_b);
                    }

                    if(config_flag(cf_led_flash_on_release)) {
                        led_flash();
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
        led_update();
    }
}
