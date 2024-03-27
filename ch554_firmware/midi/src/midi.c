//////////////////////////////////////////////////////////////////////

#include "main.h"

static uint8 device_id = 0;
static uint8 *midi_send_ptr;
static uint8 midi_send_remain = 0;
static uint8 sysex_recv_length = 0;
static uint8 sysex_recv_packet_offset = 0;

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
    sysex_hdr_t *p = (sysex_hdr_t *)midi_send_buffer;
    p->sysex_start = 0xF0;
    p->sysex_realtime = 0x7E;
    p->sysex_device_index = device_id;
    p->sysex_type = 0x07;    // machine control response
    p->sysex_code = sysex_code;
    return (void *)(midi_send_buffer + sizeof(sysex_hdr_t));
}

//////////////////////////////////////////////////////////////////////
// send the next waiting packet if there is one and the usb is ready

void midi_flush_queue()
{
    if((usb.idle & 2) == 0) {
        return;
    }

    uint8 packets;
    uint32 *dst = (uint32 *)usb_endpoint_2_tx_buffer;
    for(packets = 0; packets < USB_PACKET_SIZE / sizeof(*dst); ++packets) {
        if(QUEUE_IS_EMPTY(midi_queue)) {
            break;
        }
        QUEUE_POP(midi_queue, *dst);
        dst += 1;
    }
    if(packets != 0) {
        // hexdump("send", usb_endpoint_2_tx_buffer, packets * 4);
        usb_send(endpoint_2, packets * 4);
    }
}

//////////////////////////////////////////////////////////////////////

bool midi_send_update()
{
    if(midi_send_remain == 0) {
        return false;
    }

    while(midi_send_remain != 0 && !QUEUE_IS_FULL(midi_queue)) {

        uint8 r = midi_send_remain;
        if(r > 3) {
            r = 3;
        }
        uint8 cmd = midi_packet_start;
        if(r < 3 || midi_send_remain <= 3) {
            cmd = midi_packet_end_1 + r - 1;
        }
        uint32 packet = 0;
        uint8 *dst = (uint8 *)&packet;
        *dst++ = cmd;
        for(uint8 i = 0; i < r; ++i) {
            *dst++ = *midi_send_ptr++;
        }
        hexdump("PUT", &packet, 4);
        QUEUE_PUSH(midi_queue, packet);
        midi_send_remain -= r;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////

static bool midi_send(uint8 *midi_data, uint8 length)
{
    if(midi_send_remain != 0) {
        return false;
    }
    if(length < 4) {
        return false;
    }
    midi_send_remain = length;
    midi_send_ptr = midi_data;
    return true;
}

//////////////////////////////////////////////////////////////////////

bool midi_send_sysex(uint8 payload_length)
{
    uint8 total_length = sizeof(sysex_hdr_t) + payload_length + 1;    // +1 for 0xF7 terminator
    midi_send_buffer[total_length - 1] = 0xF7;
    return midi_send(midi_send_buffer, total_length);
}

//////////////////////////////////////////////////////////////////////

void handle_midi_packet()
{
    if(midi_recv_buffer[0] == 0xF0 &&                      // sysex status byte
       midi_recv_buffer[1] == 0x7E &&                      // non-realtime
       midi_recv_buffer[3] == 0x06 &&                      // machine control command
       midi_recv_buffer[sysex_recv_length - 1] == 0xF7)    // sysex terminator
    {
        hexdump("SYSEX", midi_recv_buffer, sysex_recv_length);

        switch(midi_recv_buffer[4]) {

        // identity request
        case sysex_request_device_id: {
            device_id = midi_recv_buffer[2];
            sysex_identity_response_t *response = init_sysex_response(sysex_response_device_id);
            response->manufacturer_id = MIDI_MANUFACTURER_ID;
            response->family_code = MIDI_FAMILY_CODE;
            response->model_number = MIDI_MODEL_NUMBER;
            response->version_number = FIRMWARE_VERSION;
            midi_send_sysex(sizeof(sysex_identity_response_t));
        } break;

        // toggle led
        case sysex_request_toggle_led:
            led_flash();
            break;

        // Get flash
        case sysex_request_get_flash: {
            load_config();
            hexdump("READ", (uint8 *)&config, sizeof(config));
            uint8 *buf = init_sysex_response(sysex_response_get_flash);
            bytes_to_bits7((uint8 *)&config, 0, sizeof(config), buf);

#define CONFIG_7BIT_LEN (((CONFIG_MAX_LEN * 8) + 6) / 7)

            midi_send_sysex(CONFIG_7BIT_LEN);
        } break;

        // Set flash
        case sysex_request_set_flash: {

            bits7_to_bytes(midi_recv_buffer, 5, CONFIG_MAX_LEN, (uint8 *)&config);
            hexdump("WRITE", (uint8 *)&config, CONFIG_MAX_LEN);
            uint8 *buf = init_sysex_response(sysex_response_set_flash_ack);
            *buf = 0x01;
            if(!save_config()) {
                puts("Error saving flash");
                *buf = 0xff;
            }
            midi_send_sysex(1);
        } break;

        case sysex_request_bootloader: {
            goto_bootloader();
        } break;
        }
    }
    sysex_recv_length = 0;
}

//////////////////////////////////////////////////////////////////////

static void sysex_parse_add(uint8 length)
{
    if(length > (sizeof(midi_recv_buffer) - sysex_recv_length)) {
        sysex_recv_length = 0;
    } else {
        memcpy(midi_recv_buffer + sysex_recv_length, usb_endpoint_2_rx_buffer + sysex_recv_packet_offset + 1, length);
        sysex_recv_length += length;
    }
}

//////////////////////////////////////////////////////////////////////

void process_midi_packet_in(uint8 length)
{
    hexdump("midi_in", usb_endpoint_2_rx_buffer, length);

    sysex_recv_packet_offset = 0;

    while(sysex_recv_packet_offset < length) {
        uint8 cmd = usb_endpoint_2_rx_buffer[sysex_recv_packet_offset];

        switch(cmd) {

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
            sysex_recv_length = 0;
            sysex_recv_packet_offset = length;
            break;
        }
        sysex_recv_packet_offset += 4;
    }
}

//////////////////////////////////////////////////////////////////////

void midi_init()
{
    puts("MIDI INIT");
    QUEUE_INIT(midi_queue);
}
