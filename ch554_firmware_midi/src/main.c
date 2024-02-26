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
#include "load_save.h"
#include "gpio.h"
#include "encoder.h"

//////////////////////////////////////////////////////////////////////
// BOOTLOADER admin

typedef void (*BOOTLOADER)(void);
#define bootloader554 ((BOOTLOADER)0x3800)    // CH551/2/3/4
#define bootloader559 ((BOOTLOADER)0xF400)    // CH558/9

// #define BOOTLOADER_DELAY 0x300    // about 3 seconds
#define BOOTLOADER_DELAY 0x80

#define DEBOUNCE_TIME 0xA0u
#define T2_DEBOUNCE (0xFFu - DEBOUNCE_TIME)

#define BOOT_FLASH_LED_COUNT 6

volatile const __code __at(ROM_CHIP_ID_LO)
uint16_t CHIP_UNIQUE_ID_LO;

volatile const __code __at(ROM_CHIP_ID_HI)
uint16_t CHIP_UNIQUE_ID_HI;

uint32 chip_id;
uint32 chip_id_28;

//////////////////////////////////////////////////////////////////////
// Rotary Encoder

int8 turn_value;

#define CLOCKWISE 2
#define ANTI_CLOCKWISE 0

// Define ROTARY_DIRECTION as CLOCKWISE for one kind of encoders, ANTI_CLOCKWISE for
// the other ones (some are reversed). This sets the default rotation (after first
// flash), reverse by triple-clicking the knob

#define ROTARY_DIRECTION (CLOCKWISE)
// #define ROTARY_DIRECTION (ANTI_CLOCKWISE)

//////////////////////////////////////////////////////////////////////

enum midi_packet_header_t
{
    midi_packet_start = 0x4,    // SysEx starts or continues with following three bytes
    midi_packet_end_1 = 0x5,    // 1 Single-byte System Common Message or SysEx ends with following single byte.
    midi_packet_end_2 = 0x6,    // SysEx ends with following two bytes.
    midi_packet_end_3 = 0x7,    // SysEx ends with following three bytes.
};

typedef uint32 midi_packet;

// must be a power of 2 and MIDI_QUEUE_LEN * sizeof(midi_packet) must be < MAX_PACKET_SIZE (64) so
// effectively the max length is 8
#define MIDI_QUEUE_LEN 8

//////////////////////////////////////////////////////////////////////

#define FLASH_7BIT_LEN (((FLASH_LEN * 8) + 6) / 7)

// XDATA, 1KB available

__xdata uint8 Ep0Buffer[DEFAULT_ENDP0_SIZE];     // endpoint0 OUT & IN buffer，Must be an even address
__xdata uint8 Ep1Buffer[DEFAULT_ENDP1_SIZE];     // endpoint1 upload buffer
__xdata uint8 Ep2Buffer[2 * MAX_PACKET_SIZE];    // endpoint2 IN & OUT buffer, Must be an even address
__xdata uint8 midi_send_buffer[48];
__xdata uint8 midi_recv_buffer[48];
__xdata save_buffer_t save_buffer;
__xdata midi_packet queue_buffer[MIDI_QUEUE_LEN];

uint8 queue_head = 0;
uint8 queue_size = 0;
uint8 device_id = 0;
uint8 *midi_send_ptr;
uint8 midi_send_remain = 0;
uint8 sysex_recv_length = 0;
uint8 sysex_recv_packet_offset = 0;

//////////////////////////////////////////////////////////////////////
// Flash LED before jumping to bootloader

void bootloader_led_flash(int8_t n)
{
    LED_BIT = 0;
    for(int8_t i = 0; i < n; ++i) {
        TF2 = 0;
        TH2 = 0;
        TL2 = 120;
        while(TF2 != 1) {
        }
        LED_BIT ^= 1;
    }
}

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

// push one onto the queue, check it's got space before calling this

void queue_put(midi_packet k)
{
    queue_buffer[(queue_head + queue_size) & (MIDI_QUEUE_LEN - 1)] = k;
    queue_size += 1;
}

// pop next from the queue, check it's not empty before calling this

midi_packet queue_get()
{
    uint8 old_head = queue_head;
    queue_size -= 1;
    queue_head = ++queue_head & (MIDI_QUEUE_LEN - 1);
    return queue_buffer[old_head];
}

// pop next into somewhere, check it's not empty before calling this

inline void queue_get_at(midi_packet *dst)
{
    *dst = queue_buffer[queue_head];
    queue_size -= 1;
    queue_head = ++queue_head & (MIDI_QUEUE_LEN - 1);
}

//////////////////////////////////////////////////////////////////////

#define MIDI_MANUFACTURER_ID 0x36    // Cheetah Marketing, defunct?
#define MIDI_FAMILY_CODE 0x5544
#define MIDI_MODEL_NUMBER 0x3322

//////////////////////////////////////////////////////////////////////

enum
{
    sysex_request_device_id = 0x01,
    sysex_request_toggle_led = 0x02,
    sysex_request_get_flash = 0x03,
    sysex_request_set_flash = 0x04
};

enum
{
    sysex_response_unused_01 = 0x01,
    sysex_response_device_id = 0x02,
    sysex_response_get_flash = 0x03,
    sysex_response_set_flash_ack = 0x04
};

typedef struct sysex_hdr
{
    uint8 sysex_start;           // 0xF0
    uint8 sysex_realtime;        // 0x7F (realtime) or 0x7E (non-realtime)
    uint8 sysex_device_index;    // get it from the identity request
    uint8 sysex_type;            // 0x06
    uint8 sysex_code;            // see enums above
} sysex_hdr_t;

typedef struct sysex_identity_response
{
    uint8 manufacturer_id;
    uint16 family_code;
    uint16 model_number;
    uint32 unique_id;
} sysex_identity_response_t;

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

void midi_packet_send_update()
{
    if(ep2_busy) {
        return;
    }
    uint8 filled = 0;
    midi_packet *dst = (midi_packet *)(Ep2Buffer + MAX_PACKET_SIZE);
    while(filled < (MAX_PACKET_SIZE - (sizeof(midi_packet) - 1)) && !queue_empty()) {
        queue_get_at(dst);
        filled += 4;
        dst += 1;
    }
    if(filled != 0) {
        hexdump("send", Ep2Buffer + MAX_PACKET_SIZE, filled);
        UEP2_T_LEN = filled;
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
        midi_packet packet = 0;
        uint8 *dst = (uint8 *)&packet;
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

bool midi_send(uint8 *data, uint8 length)
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
            if(!load_flash(&save_buffer)) {
                memset(save_buffer.data, 0xff, sizeof(save_buffer.data));
            }
            hexdump("READ", save_buffer.data, FLASH_LEN);
            uint8 *buf = init_sysex_response(sysex_response_get_flash);
            bytes_to_bits7(save_buffer.data, 0, FLASH_LEN, buf);
            midi_send_sysex(FLASH_7BIT_LEN);
        } break;

        // Set flash
        case sysex_request_set_flash: {
            bits7_to_bytes(midi_recv_buffer, 5, FLASH_LEN, save_buffer.data);
            hexdump("WRITE", save_buffer.data, FLASH_LEN);
            uint8 *buf = init_sysex_response(sysex_response_set_flash_ack);
            *buf = 0x01;
            if(!save_flash(&save_buffer)) {
                putstr("Error saving flash\n");
                *buf = 0xff;
            }
            midi_send_sysex(1);
        } break;
        }
    }
    sysex_recv_length = 0;
}

//////////////////////////////////////////////////////////////////////

void sysex_parse_add(uint8 length)
{
    if(length > (sizeof(midi_recv_buffer) - sysex_recv_length)) {
        sysex_recv_length = 0;
    } else {
        memcpy(midi_recv_buffer + sysex_recv_length, Ep2Buffer + sysex_recv_packet_offset + 1, length);
        sysex_recv_length += length;
    }
}

//////////////////////////////////////////////////////////////////////

void process_midi_packet_in(uint8 length)
{
    sysex_recv_packet_offset = 0;

    while(sysex_recv_packet_offset < length) {

        uint8 cmd = Ep2Buffer[sysex_recv_packet_offset];

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

int main()
{
    uint16_t press_time = 0;

    CfgFsys();       // CH559 clock select configuration
    mDelaymS(5);     // Modify the main frequency and wait for the internal crystal stability, it will be added
    UART0_Init();    // Candidate 0, can be used for debugging

    // get unique chip id
    chip_id = CHIP_UNIQUE_ID_LO | ((uint32)CHIP_UNIQUE_ID_HI << 16);

    // make a 28 bit version so we can send as 4 x 7bits for midi identification
    uint8 *cip = (uint8 *)&chip_id_28;
    cip[0] = (chip_id >> 21) & 0x7f;
    cip[1] = (chip_id >> 14) & 0x7f;
    cip[2] = (chip_id >> 7) & 0x7f;
    cip[3] = (chip_id >> 0) & 0x7f;

    hexdump("===== CHIPID =====", &chip_id, 4);

    gpio_init(UART_TX_PORT, UART_TX_PIN, gpio_output_push_pull);
    gpio_init(UART_RX_PORT, UART_RX_PIN, gpio_output_open_drain);
    gpio_init(ROTA_PORT, ROTA_PIN, gpio_input_pullup);
    gpio_init(ROTB_PORT, ROTB_PIN, gpio_input_pullup);
    gpio_init(BTN_PORT, BTN_PIN, gpio_input_pullup);

    // init usb
    usb_device_config();
    usb_device_endpoint_config();    // Endpoint configuration
    usb_device_int_config();         // Interrupt initialization
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;    // Be pre -use and sending length must be empty
    UEP2_T_LEN = 0;    // Be pre -use and sending length must be empty

    // flash the led a few times at boot

    TR0 = 1;
    TR2 = 1;

    for(uint8 i = 0; i < BOOT_FLASH_LED_COUNT; ++i) {
        while(TF2 == 0) {
        }
        TL2 = 0;
        TH2 = 0;
        TF2 = 0;
        LED_BIT = !LED_BIT;
    }

    LED_BIT = 0;

    bool button_state = false;    // for debouncing the button

    turn_value = ROTARY_DIRECTION - 1;

    while(1) {

        // read/debounce the button
        bool pressed = false;
        bool new_state = !BTN_BIT;

        if(new_state != button_state && TF2) {
            TL2 = 0;
            TH2 = T2_DEBOUNCE;
            TF2 = 0;
            pressed = new_state;
            button_state = new_state;
        }

        if(button_state) {
            if(TF0 == 1) {
                TF0 = 0;
                press_time += 1;
                if(press_time == BOOTLOADER_DELAY) {

                    // shutdown peripherals
                    EA = 0;
                    USB_CTRL = 0;
                    UDEV_CTRL = 0;

                    // flash LED for a bit
                    bootloader_led_flash(8);

                    // and jump to bootloader
                    bootloader554();
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

            // send any waiting midi packets
            midi_packet_send_update();

            // maybe add some more to the queue
            if(!queue_full() && !midi_send_update()) {

                if(pressed) {

                    queue_put(0x0103b00b);
                    LED_BIT = !LED_BIT;
                }
                if(!queue_full()) {
                    if(direction == turn_value) {

                        queue_put(0x0104b00b);
                        LED_BIT = !LED_BIT;

                    } else if(direction == -turn_value) {

                        queue_put(0x0105b00b);
                        LED_BIT = !LED_BIT;
                    }
                }
            }
        }
    }
}
