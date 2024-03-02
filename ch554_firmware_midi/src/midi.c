#include <string.h>
#include "ch554.h"
#include "types.h"
#include "debug.h"
#include "midi.h"
#include "config.h"
#include "xdata.h"
#include "usb.h"
#include "util.h"
#include "gpio.h"
#include "main.h"

uint8 queue_size = 0;

static uint8 queue_head = 0;
static uint8 device_id = 0;
static uint8 *midi_send_ptr;
static uint8 midi_send_remain = 0;
static uint8 sysex_recv_length = 0;
static uint8 sysex_recv_packet_offset = 0;

//////////////////////////////////////////////////////////////////////
// push one onto the queue, check it's got space before calling this

void queue_put(uint8 *k)
{
    uint8 const offset = ((queue_head + queue_size) & (MIDI_QUEUE_LEN - 1)) * MIDI_PACKET_SIZE;
    memcpy(queue_buffer + offset, k, MIDI_PACKET_SIZE);
    queue_size += 1;
}

// //////////////////////////////////////////////////////////////////////
// // pop next from the queue, check it's not empty before calling this

// static midi_packet queue_get()
// {
//     uint8 old_head = queue_head;
//     queue_size -= 1;
//     queue_head = ++queue_head & (MIDI_QUEUE_LEN - 1);
//     return queue_buffer[old_head];
// }

//////////////////////////////////////////////////////////////////////
// pop next into somewhere, check it's not empty before calling this

static inline void queue_get_at(uint8 *dst)
{
    memcpy(dst, queue_buffer + (queue_head * MIDI_PACKET_SIZE), MIDI_PACKET_SIZE);
    queue_size -= 1;
    queue_head = ++queue_head & (MIDI_QUEUE_LEN - 1);
}

//////////////////////////////////////////////////////////////////////

void *init_sysex_response(uint8 code)
{
    sysex_hdr_t *p = (sysex_hdr_t *)midi_send_buffer;
    p->sysex_start = 0xF0;
    p->sysex_realtime = 0x7E;
    p->sysex_device_index = device_id;
    p->sysex_type = 0x07;    // machine control response
    p->sysex_code = code;
    return (void *)(midi_send_buffer + sizeof(sysex_hdr_t));
}

//////////////////////////////////////////////////////////////////////
// send the next waiting packet if there is one and the usb is ready

void midi_flush_queue()
{
    if(ep2_busy) {
        return;
    }

    uint8 *dst;
    for(dst = endpoint_2_out_buffer; !queue_empty() && dst < (endpoint_2_out_buffer + MAX_PACKET_SIZE); dst += 4) {
        queue_get_at(dst);
    }
    if(dst != endpoint_2_out_buffer) {
        hexdump("send", endpoint_2_out_buffer, dst - endpoint_2_out_buffer);
        UEP2_T_LEN = dst - endpoint_2_out_buffer;
        UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;    // Answer ACK
        ep2_busy = 1;
    }
}

//////////////////////////////////////////////////////////////////////

bool midi_send_update()
{
    if(midi_send_remain == 0) {
        return false;
    }
    while(!queue_full() && midi_send_remain != 0) {
        uint8 r = midi_send_remain;
        if(r > 3) {
            r = 3;
        }
        uint8 cmd = midi_packet_start;
        if(r < 3 || midi_send_remain <= 3) {
            cmd = midi_packet_end_1 + r - 1;
        }
        uint8 packet[MIDI_PACKET_SIZE];
        uint8 *dst = packet;
        *dst++ = cmd;
        for(uint8 i = 0; i < r; ++i) {
            *dst++ = *midi_send_ptr++;
        }
        queue_put(packet);
        midi_send_remain -= r;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////

static bool midi_send(uint8 *data, uint8 length)
{
    if(midi_send_remain != 0) {
        return false;
    }
    if(length < 4) {
        return false;
    }
    midi_send_remain = length;
    midi_send_ptr = data;
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

        hexdump("MIDI", midi_recv_buffer, sysex_recv_length);

        switch(midi_recv_buffer[4]) {

        // identity request
        case sysex_request_device_id: {
            device_id = midi_recv_buffer[2];
            sysex_identity_response_t *response = init_sysex_response(sysex_response_device_id);
            response->manufacturer_id = MIDI_MANUFACTURER_ID;
            response->family_code = MIDI_FAMILY_CODE;
            response->model_number = MIDI_MODEL_NUMBER;
            response->unique_id = chip_id_28;
            midi_send_sysex(sizeof(sysex_identity_response_t));
        } break;

        // toggle led
        case sysex_request_toggle_led:
            LED_BIT = !LED_BIT;
            break;

        // Get flash
        case sysex_request_get_flash: {
            load_config();
            hexdump("READ", (uint8 *)&config, sizeof(config));
            uint8 *buf = init_sysex_response(sysex_response_get_flash);
            bytes_to_bits7((uint8 *)&config, 0, sizeof(config), buf);
            midi_send_sysex(FLASH_7BIT_LEN);
        } break;

        // Set flash
        case sysex_request_set_flash: {

            bits7_to_bytes(midi_recv_buffer, 5, FLASH_LEN, (uint8 *)&config);
            hexdump("WRITE", (uint8 *)&config, FLASH_LEN);
            uint8 *buf = init_sysex_response(sysex_response_set_flash_ack);
            *buf = 0x01;
            if(!save_config()) {
                putstr("Error saving flash\n");
                *buf = 0xff;
            }
            midi_send_sysex(1);
        } break;

        case sysex_request_bootloader: {
            goto_bootloader();
        } break;

        case sysex_request_firmware_version: {
            uint8 *buf = init_sysex_response(sysex_response_firmware_version);
            *buf++ = FIRMWARE_VERSION & 0xff;
            *buf++ = FIRMWARE_VERSION >> 8;
            midi_send_sysex(2);
        };
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
        memcpy(midi_recv_buffer + sysex_recv_length, endpoint_2_in_buffer + sysex_recv_packet_offset + 1, length);
        sysex_recv_length += length;
    }
}

//////////////////////////////////////////////////////////////////////

void process_midi_packet_in(uint8 length)
{
    sysex_recv_packet_offset = 0;

    while(sysex_recv_packet_offset < length) {

        uint8 cmd = endpoint_2_in_buffer[sysex_recv_packet_offset];

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
