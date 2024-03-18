#pragma once

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
    midi_packet_start = 0x4,    // SysEx starts or continues with following three bytes
    midi_packet_end_1 = 0x5,    // 1 Single-byte System Common Message or SysEx ends with following single byte.
    midi_packet_end_2 = 0x6,    // SysEx ends with following two bytes.
    midi_packet_end_3 = 0x7,    // SysEx ends with following three bytes.
};

extern uint8 queue_size;

//////////////////////////////////////////////////////////////////////
// a queue of MIDI packets
// we have to queue things up because the rotary encoder can generate
// messages faster than we can send them

inline bool queue_full()
{
    return queue_size == MIDI_QUEUE_LEN;
}

inline uint8 queue_space()
{
    return MIDI_QUEUE_LEN - queue_size;
}

inline bool queue_empty()
{
    return queue_size == 0;
}


void process_midi_packet_in(uint8 length);
void handle_midi_packet();
void midi_flush_queue();
bool midi_send_update();

void queue_put(uint8 *k);