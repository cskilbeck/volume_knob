//////////////////////////////////////////////////////////////////////

#include "main.h"

#define XDATA __xdata
#include "xdata_extra.h"
#undef XDATA

//////////////////////////////////////////////////////////////////////

#define MIDI_MANUFACTURER_ID 0x36    // Cheetah Marketing, defunct?
#define MIDI_FAMILY_CODE 0x5544
#define MIDI_MODEL_NUMBER 0x3322

//////////////////////////////////////////////////////////////////////
// requests

enum
{
    sysex_request_device_id = 0x01,
    sysex_request_toggle_led = 0x02,
    sysex_request_get_flash = 0x03,
    sysex_request_set_flash = 0x04,
    sysex_request_bootloader = 0x05
};

//////////////////////////////////////////////////////////////////////
// responses

enum
{
    sysex_response_unused_01 = 0x01,
    sysex_response_device_id = 0x02,
    sysex_response_get_flash = 0x03,
    sysex_response_set_flash_ack = 0x04
};

//////////////////////////////////////////////////////////////////////

typedef struct sysex_hdr
{
    uint8 sysex_start;           // 0xF0
    uint8 sysex_realtime;        // 0x7F (realtime) or 0x7E (non-realtime)
    uint8 sysex_device_index;    // get it from the identity request
    uint8 sysex_type;            // 0x06
    uint8 sysex_code;            // see enums above
} sysex_hdr_t;

//////////////////////////////////////////////////////////////////////

typedef struct sysex_identity_response
{
    uint8 manufacturer_id;
    uint16 family_code;
    uint16 model_number;
    uint32 version_number;
} sysex_identity_response_t;

//////////////////////////////////////////////////////////////////////

enum midi_packet_header_t
{
    midi_packet_start = 0x04,    // SysEx starts or continues with following three bytes
    midi_packet_end_1 = 0x05,    // 1 Single-byte System Common Message or SysEx ends with following single byte.
    midi_packet_end_2 = 0x06,    // SysEx ends with following two bytes.
    midi_packet_end_3 = 0x07,    // SysEx ends with following three bytes.
    midi_packet_cc = 0x0b,       // Change Control msg
};

//////////////////////////////////////////////////////////////////////

static uint8 device_id = 0;
static uint8 *midi_sysex_ptr;
static uint8 midi_sysex_remain = 0;
static uint8 recv_length = 0;
static uint8 recv_packet_offset = 0;

#define CONFIG_7BIT_LEN (((CONFIG_MAX_LEN * 8) + 6) / 7)

//////////////////////////////////////////////////////////////////////
// expand some bytes into an array of 7 bit values

static void bytes_to_bits7(uint8 *src_data, uint8 offset, uint8 len, uint8 *dest)
{
    uint8 bits_available = 0;
    uint16 cur_src = 0;
    uint8 bits_remaining = len * 8;
    uint8 dest_index = 0;

    while(true) {

        if(bits_available < 7) {
            cur_src = (cur_src << 8) | src_data[offset++];
            bits_available += 8;
        }

        while(bits_available >= 7) {
            bits_available -= 7;
            dest[dest_index++] = (cur_src >> bits_available) & 0x7f;
            bits_remaining -= 7;
        }

        if(bits_remaining < 7) {
            dest[dest_index++] = (cur_src << (7 - bits_remaining)) & 0x7f;
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// convert some 7 bit values to bytes

static void bits7_to_bytes(uint8 *src_data, uint8 offset, uint8 len, uint8 *dest)
{
    uint8 bits_available = 0;
    uint16 cur_src = 0;
    uint8 dest_index = 0;

    while(dest_index < len) {

        if(bits_available < 8) {
            cur_src = (cur_src << 7) | src_data[offset++];
            bits_available += 7;
        }

        while(bits_available >= 8) {
            bits_available -= 8;
            dest[dest_index++] = (cur_src >> bits_available) & 0xff;
        }
    }
}

//////////////////////////////////////////////////////////////////////

void *init_sysex_response(uint8 sysex_code)
{
    if(midi_sysex_remain != 0) {
        return NULL;
    }

    sysex_hdr_t *p = (sysex_hdr_t *)midi_send_buffer;
    p->sysex_start = 0xF0;
    p->sysex_realtime = 0x7E;
    p->sysex_device_index = device_id;
    p->sysex_type = 0x07;    // machine control response
    p->sysex_code = sysex_code;
    return (void *)(midi_send_buffer + sizeof(sysex_hdr_t));
}

//////////////////////////////////////////////////////////////////////
// send the next waiting packet(s) if there are any and the usb is ready

void midi_flush_queue()
{
    if(usb_is_endpoint_idle(endpoint_2)) {

        uint32 *end = (uint32 *)(usb_endpoint_2_tx_buffer + USB_PACKET_SIZE);

        uint32 *dst = (uint32 *)usb_endpoint_2_tx_buffer;

        for(; dst < end && !QUEUE_IS_EMPTY(midi_queue); ++dst) {

            QUEUE_POP(midi_queue, *dst);
        }

        uint8 num_bytes = (uint8 *)dst - usb_endpoint_2_tx_buffer;

        if(num_bytes != 0) {

            usb_send(endpoint_2, num_bytes);
        }
    }
}

//////////////////////////////////////////////////////////////////////
// send as much midi sysex data as we can

void midi_send_update()
{
    while(midi_sysex_remain != 0 && !QUEUE_IS_FULL(midi_queue)) {

        uint8 r = midi_sysex_remain;
        if(r > 3) {
            r = 3;
        }
        uint8 cmd = midi_packet_start;
        if(r < 3 || midi_sysex_remain <= 3) {
            cmd = midi_packet_end_1 + r - 1;
        }
        uint32 packet = 0;
        uint8 *dst = (uint8 *)&packet;
        *dst++ = cmd;
        for(uint8 i = 0; i < r; ++i) {
            *dst++ = *midi_sysex_ptr++;
        }
        hexdump("PUT", &packet, 4);
        QUEUE_PUSH(midi_queue, packet);
        midi_sysex_remain -= r;
    }
}

//////////////////////////////////////////////////////////////////////

bool midi_send_sysex(uint8 payload_length)
{
    uint8 total_length = sizeof(sysex_hdr_t) + payload_length + 1;    // +1 for 0xF7 terminator
    midi_send_buffer[total_length - 1] = 0xF7;
    midi_sysex_remain = total_length;
    midi_sysex_ptr = midi_send_buffer;
    return true;
}

//////////////////////////////////////////////////////////////////////
// Handle a sysex midi message

void handle_sysex_in()
{
    if(midi_recv_buffer[0] == 0xF0 &&                      // sysex status byte
       midi_recv_buffer[1] == 0x7E &&                      // non-realtime
       midi_recv_buffer[3] == 0x06 &&                      // machine control command
       midi_recv_buffer[recv_length - 1] == 0xF7)    // sysex terminator
    {
        hexdump("SYSEX", midi_recv_buffer, recv_length);

        switch(midi_recv_buffer[4]) {

        // identity request
        case sysex_request_device_id: {
            device_id = midi_recv_buffer[2];
            sysex_identity_response_t *response = init_sysex_response(sysex_response_device_id);
            if(response != NULL) {
                response->manufacturer_id = MIDI_MANUFACTURER_ID;
                response->family_code = MIDI_FAMILY_CODE;
                response->model_number = MIDI_MODEL_NUMBER;
                response->version_number = FIRMWARE_VERSION;
                midi_send_sysex(sizeof(sysex_identity_response_t));
            }
        } break;

        // toggle led
        case sysex_request_toggle_led: {
            led_flash();
        } break;

        // Get flash
        case sysex_request_get_flash: {
            load_config();
            hexdump("READ Flash", (uint8 *)&midi_config, sizeof(midi_config));
            uint8 *buf = init_sysex_response(sysex_response_get_flash);
            if(buf != NULL) {
                bytes_to_bits7((uint8 *)&midi_config, 0, sizeof(midi_config), buf);
                midi_send_sysex(CONFIG_7BIT_LEN);
            }
        } break;

        // Set flash
        case sysex_request_set_flash: {

            bits7_to_bytes(midi_recv_buffer, 5, CONFIG_MAX_LEN, (uint8 *)&midi_config);
            hexdump("WRITE Flash", (uint8 *)&midi_config, CONFIG_MAX_LEN);
            uint8 *buf = init_sysex_response(sysex_response_set_flash_ack);
            if(buf != NULL) {
                *buf = 0x01;
                if(!save_config()) {
                    puts("Error saving flash");
                    *buf = 0xff;
                }
                midi_send_sysex(1);
            }
        } break;

        case sysex_request_bootloader: {
            goto_bootloader();
        } break;
        }
    }
    recv_length = 0;
}

//////////////////////////////////////////////////////////////////////

static void sysex_parse_add(uint8 length)
{
    if(length > (sizeof(midi_recv_buffer) - recv_length)) {
        recv_length = 0;
    } else {
        memcpy(midi_recv_buffer + recv_length, usb_endpoint_2_rx_buffer + recv_packet_offset + 1, length);
        recv_length += length;
    }
}

//////////////////////////////////////////////////////////////////////

static void cc_parse()
{
    uint8 channel = usb_endpoint_2_rx_buffer[1];
    if((channel & 0xB0) == 0xB0) {
        channel &= 0x0F;
        uint8 param = usb_endpoint_2_rx_buffer[2];
        uint8 value = usb_endpoint_2_rx_buffer[3];
        if(channel == get_rot_channel()) {
            if(config_flag(cf_btn_extended)) {
                uint16 cur = midi_config.rot_current_value_14;
                if(param == midi_config.rot_control_msb) {
                    cur &= 0x007f;
                    cur |= value << 7;
                    midi_config.rot_current_value_14 = cur;
                    printf("MSB=%d\n", value);
                } else if(param == midi_config.rot_control_lsb) {
                    cur &= 0xff80;
                    cur |= value;
                    midi_config.rot_current_value_14 = cur;
                    printf("LSB=%d\n", value);
                }
            } else if(param == midi_config.rot_control_msb) {
                midi_config.rot_current_value_7 = value;
                printf("VAL=%d\n", value);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////

void process_midi_packet_in(uint8 length)
{
    hexdump("midi_in", usb_endpoint_2_rx_buffer, length);

    recv_packet_offset = 0;

    while(recv_packet_offset < length) {

        uint8 cmd = usb_endpoint_2_rx_buffer[recv_packet_offset];

        switch(cmd) {

        case midi_packet_cc:
            cc_parse();
            break;

        case midi_packet_start:
        case midi_packet_end_3:
            sysex_parse_add(3);
            break;

        case midi_packet_end_1:
            sysex_parse_add(1);
            break;

        case midi_packet_end_2:
            sysex_parse_add(2);
            break;

        default:
            recv_length = 0;
            recv_packet_offset = length;
            break;
        }
        recv_packet_offset += 4;
    }
    handle_sysex_in();
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
    uint8 space_needed = is_extended ? 2 : 1;

    if(QUEUE_SPACE(midi_queue) < space_needed) {
        printf("send cc: queue full (need %d)\n", space_needed);
        return;
    }

    uint32 midi_packet;

#define _pkt ((uint8 *)&midi_packet)

    // CC header
    _pkt[0] = 0x0B;
    _pkt[1] = 0xB0 | channel;

    // cc[0] is the value to send (or MSB of value)
    _pkt[2] = cc_msb;

    // send MSB first if extended mode
    if(is_extended) {

        // send MSB of value
        _pkt[3] = (value >> 7) & 0x7F;
        QUEUE_PUSH(midi_queue, midi_packet);

        // cc[1] is LSB of value
        _pkt[2] = cc_lsb;
    }

    // send value (or LSB of value)
    _pkt[3] = value & 0x7f;
    QUEUE_PUSH(midi_queue, midi_packet);

#undef _pkt
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

    uint16 value = extended ? midi_config.btn_value_a_14 : midi_config.btn_value_a_7;

    if(val == value_b) {

        value = extended ? midi_config.btn_value_b_14 : midi_config.btn_value_b_7;
    }
    send_cc(get_btn_channel(), midi_config.btn_control_msb, midi_config.btn_control_lsb, value, extended);
}

//////////////////////////////////////////////////////////////////////

static int16 rotation_velocity = 0;
static uint8 deceleration_ticks = 0;

void do_absolute_rotation(int16 offset)
{
    bool extended = config_flag(cf_rotate_extended);
    bool at_limit = false;
    int32 cur = extended ? midi_config.rot_current_value_14 : midi_config.rot_current_value_7;
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
        midi_config.rot_current_value_14 = (int16)cur;
    } else {
        midi_config.rot_current_value_7 = (int16)cur;
    }
    send_cc(get_rot_channel(), midi_config.rot_control_msb, midi_config.rot_control_lsb, cur, extended);

    if(config_flag(cf_led_flash_on_rot) || (at_limit && config_flag(cf_led_flash_on_limit))) {

        led_flash();
    }
}

//////////////////////////////////////////////////////////////////////
// relative rotation is forced 7 bit mode

void do_relative_rotation(int16 offset)
{
    uint8 cur = (midi_config.rot_zero_point + offset * (rotation_velocity + 1)) & 0x7f;
    send_cc(get_rot_channel(), midi_config.rot_control_msb, 0, cur, false);

    if(config_flag(cf_led_flash_on_rot)) {
        led_flash();
    }
}

//////////////////////////////////////////////////////////////////////

void midi_init()
{
    puts("MIDI INIT");
    QUEUE_INIT(midi_queue);
}

//////////////////////////////////////////////////////////////////////

void midi_tick()
{
    deceleration_ticks += 1;
}

//////////////////////////////////////////////////////////////////////

void midi_usb_receive(uint8 len)
{
    process_midi_packet_in(len);
}

//////////////////////////////////////////////////////////////////////

void midi_update()
{
    midi_flush_queue();
    midi_send_update();

    if(deceleration_ticks == 50) {

        deceleration_ticks = 0;
        rotation_velocity >>= 1;
    }
}

//////////////////////////////////////////////////////////////////////

void midi_rotate(int8 direction)
{
    // direction is backwards so negate it _unless_ cf_rotate_reverse
    if(!config_flag(cf_rotate_reverse)) {
        direction = -direction;
    }

    int16 delta = config_flag(cf_rotate_extended) ? midi_config.rot_delta_14 : midi_config.rot_delta_7;

    if(direction == 1) {
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
}

//////////////////////////////////////////////////////////////////////

void midi_press()
{
    if(midi_sysex_remain != 0) {
        return;
    }

    if(QUEUE_IS_FULL(midi_queue)) {
        return;
    }

    if(config_flag(cf_led_flash_on_click)) {
        led_flash();
    }

    midi_config.flags ^= cf_toggle;

    which_value_t value = value_a;

    if(!config_flag(cf_btn_momentary) && config_flag(cf_toggle)) {
        value = value_b;
    }
    send_button_cc(value);
}

//////////////////////////////////////////////////////////////////////

void midi_release()
{
    if(midi_sysex_remain != 0) {
        return;
    }

    if(QUEUE_IS_FULL(midi_queue)) {
        return;
    }

    if(config_flag(cf_led_flash_on_release)) {
        led_flash();
    }

    if(config_flag(cf_btn_momentary)) {
        send_button_cc(value_b);
    }
}

//////////////////////////////////////////////////////////////////////

__code const process_t midi_process = { .process_name = "MIDI",
                                        .on_init = midi_init,
                                        .on_tick = midi_tick,
                                        .on_update = midi_update,
                                        .on_rotate = midi_rotate,
                                        .on_press = midi_press,
                                        .on_release = midi_release,
                                        .on_usb_receive = { NULL, midi_usb_receive, NULL, NULL } };

__code const process_t *current_process = &midi_process;
