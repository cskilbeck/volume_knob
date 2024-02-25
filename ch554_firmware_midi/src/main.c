//////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ch554.h"
#include "ch554_usb.h"
#include "debug.h"
#include "util.h"
#include "usb.h"

//////////////////////////////////////////////////////////////////////
// BOOTLOADER admin

typedef void (*BOOTLOADER)(void);
#define bootloader554 ((BOOTLOADER)0x3800)    // CH551/2/3/4
#define bootloader559 ((BOOTLOADER)0xF400)    // CH558/9

#define BOOTLOADER_DELAY 0x300    // about 3 seconds

volatile const __code __at(ROM_CHIP_ID_LO)
uint16_t CHIP_UNIQUE_ID_LO;

volatile const __code __at(ROM_CHIP_ID_HI)
uint16_t CHIP_UNIQUE_ID_HI;

uint32 chip_id;

//////////////////////////////////////////////////////////////////////
// GPIO

#define PORT1 0x90
#define PORT3 0xB0

#define UART_TX_PORT PORT3
#define UART_TX_PIN 0

#define UART_RX_PORT PORT3
#define UART_RX_PIN 1

#define ROTA_PORT PORT3
#define ROTA_PIN 3

#define ROTB_PORT PORT3
#define ROTB_PIN 4

#define BTN_PORT PORT3
#define BTN_PIN 2

#define LED_PORT PORT1
#define LED_PIN 6

SBIT(BTN_BIT, BTN_PORT, BTN_PIN);
SBIT(LED_BIT, LED_PORT, LED_PIN);
SBIT(ROTA_BIT, ROTA_PORT, ROTA_PIN);
SBIT(ROTB_BIT, ROTB_PORT, ROTB_PIN);

//////////////////////////////////////////////////////////////////////
// Rotary Encoder

uint8 vol_direction;
int8 turn_value;

#define CLOCKWISE 2
#define ANTI_CLOCKWISE 0

// Define ROTARY_DIRECTION as CLOCKWISE for one kind of encoders, ANTI_CLOCKWISE for
// the other ones (some are reversed). This sets the default rotation (after first
// flash), reverse by triple-clicking the knob

#define ROTARY_DIRECTION (CLOCKWISE)
// #define ROTARY_DIRECTION (ANTI_CLOCKWISE)

//////////////////////////////////////////////////////////////////////

enum midi_code_index
{
    mci_sysex_start = 0x4,    // 3 SysEx starts or continues
    mci_sysex_end_1 = 0x5,    // 1 Single-byte System Common Message or SysEx ends with following single byte.
    mci_sysex_end_2 = 0x6,    // 2 SysEx ends with following two bytes.
    mci_sysex_end_3 = 0x7,    // 3 SysEx ends with following three bytes.
};

//////////////////////////////////////////////////////////////////////

#define FLASH_LEN 16
#define FLASH_7BIT_LEN (((FLASH_LEN * 8) + 6) / 7)

__xdata __at(0x0080 + sizeof(Ep2Buffer)) uint8 send_buffer[48];
__xdata __at(0x0080 + sizeof(Ep2Buffer) + sizeof(send_buffer)) uint8 flash_buffer[FLASH_LEN];

typedef uint32 midi_packet;

// must be a power of 2
#define KEY_QUEUE_LEN 4

__idata midi_packet queue_buffer[KEY_QUEUE_LEN];
__idata uint8 queue_head = 0;
__idata uint8 queue_size = 0;

__idata uint8 sysex_recv_buffer[48];
__idata uint8 sysex_recv_length = 0;
__idata uint8 sysex_recv_packet_offset = 0;
__idata uint8 device_id = 0;

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
    return queue_size == KEY_QUEUE_LEN;
}

inline uint8 queue_space()
{
    return KEY_QUEUE_LEN - queue_size;
}

inline bool queue_empty()
{
    return queue_size == 0;
}

// push one onto the queue, check it's got space before calling this

void queue_put(midi_packet k)
{
    queue_buffer[(queue_head + queue_size) & (KEY_QUEUE_LEN - 1)] = k;
    queue_size += 1;
}

// pop next from the queue, check it's not empty before calling this

midi_packet queue_get()
{
    uint8 old_head = queue_head;
    queue_size -= 1;
    queue_head = ++queue_head & (KEY_QUEUE_LEN - 1);
    return queue_buffer[old_head];
}

// pop next into somewhere, check it's not empty before calling this

inline void queue_get_at(midi_packet *dst)
{
    *dst = queue_buffer[queue_head];
    queue_size -= 1;
    queue_head = ++queue_head & (KEY_QUEUE_LEN - 1);
}

//////////////////////////////////////////////////////////////////////
// send the next waiting packet if there is one and the usb is ready

void midi_packet_send_update()
{
    if(!ep2_busy && !queue_empty()) {
        queue_get_at((uint32 *)(Ep2Buffer + MAX_PACKET_SIZE));
        hexdump("send", Ep2Buffer + MAX_PACKET_SIZE, 4);
        UEP2_T_LEN = 4;
        UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;    // Answer ACK
        ep2_busy = 1;
    }
}

//////////////////////////////////////////////////////////////////////

#define MIDI_MANUFACTURER_ID 0x36    // Cheetah Marketing, defunct
#define MIDI_FAMILY_CODE_LOW 0x55
#define MIDI_FAMILY_CODE_HIGH 0x44
#define MIDI_MODEL_NUMBER_LOW 0x33
#define MIDI_MODEL_NUMBER_HIGH 0x22

//////////////////////////////////////////////////////////////////////

__code uint8 const identity_request_2[] = { 0xF0, 0x7E };
// channel in between, we ignore it
__code uint8 const identity_request_3[] = { 0x06, 0x01, 0xF7 };

#define IDENTITY_REQUEST_LENGTH (sizeof(identity_request_2) + 1 + sizeof(identity_request_3))

uint8 const identity_response[] = {
    0xF0,    // identity response header
    0x7E,    // identity response header
    0x00,    // identity response header
    0x06,    // identity response header
    0x02,    // identity response header
    MIDI_MANUFACTURER_ID,
    MIDI_FAMILY_CODE_LOW,
    MIDI_FAMILY_CODE_HIGH,
    MIDI_MODEL_NUMBER_LOW,
    MIDI_MODEL_NUMBER_HIGH,
    0x00,    // UNIQUE_ID goes in here
    0x00,    // UNIQUE_ID goes in here
    0x00,    // UNIQUE_ID goes in here
    0x00,    // UNIQUE_ID goes in here
    0xF7     // sysex terminator
};

//////////////////////////////////////////////////////////////////////

uint8 *usb_send_ptr;
uint8 usb_sent = 0;
uint8 usb_send_remain = 0;

bool usb_send_update()
{
    uint8 r = usb_send_remain;
    if(r == 0) {
        return false;
    }

    if(r > 3) {
        r = 3;
    }
    uint8 cmd = mci_sysex_start;
    if(r < 3 || usb_send_remain <= 3) {
        cmd = mci_sysex_end_1 + r - 1;
    }
    midi_packet packet = 0;
    uint8 *dst = (uint8 *)&packet;
    uint8 *src = usb_send_ptr;
    *dst++ = cmd;
    for(uint8 i = 0; i < r; ++i) {
        *dst++ = *src++;
    }
    queue_put(packet);
    usb_sent += r;
    usb_send_ptr += r;
    usb_send_remain -= r;
    return true;
}

//////////////////////////////////////////////////////////////////////

bool usb_send(uint8 *data, uint8 length)
{
    if(usb_send_remain != 0) {
        return false;
    }
    if(length < 4) {
        return false;
    }
    usb_send_remain = length;
    usb_send_ptr = data;
    return true;
}

//////////////////////////////////////////////////////////////////////

void sysex_parse_add(uint8 length)
{
    if(length > (sizeof(sysex_recv_buffer) - sysex_recv_length)) {
        sysex_recv_length = 0;
    } else {
        memcpy(sysex_recv_buffer + sysex_recv_length, Ep2Buffer + sysex_recv_packet_offset + 1, length);
        sysex_recv_length += length;
    }
}

//////////////////////////////////////////////////////////////////////

void handle_midi_packet()
{
    if(sysex_recv_buffer[sysex_recv_length - 1] == 0xF7) {

        hexdump("MIDI", sysex_recv_buffer, sysex_recv_length);

        if(sysex_recv_buffer[0] == 0xF0 && sysex_recv_buffer[1] == 0x7E && sysex_recv_buffer[3] == 0x06) {

            switch(sysex_recv_buffer[4]) {

            // identity request
            case 0x01:
                device_id = sysex_recv_buffer[2];
                memcpy(send_buffer, identity_response, sizeof(identity_response));
                send_buffer[2] = device_id;
                send_buffer[10] = (chip_id >> 21) & 0x7f;
                send_buffer[11] = (chip_id >> 14) & 0x7f;
                send_buffer[12] = (chip_id >> 7) & 0x7f;
                send_buffer[13] = (chip_id >> 0) & 0x7f;
                usb_send(send_buffer, sizeof(identity_response));
                break;

            // toggle led
            case 0x02:
                LED_BIT = !LED_BIT;
                break;

            // Get flash
            case 0x03: {
                read_flash_data(0, FLASH_LEN, flash_buffer);
                hexdump("READ", flash_buffer, FLASH_LEN);
                send_buffer[2] = device_id;
                send_buffer[4] = 0x3;
                memset(send_buffer + 5, 0, FLASH_7BIT_LEN);
                bytes_to_bits7(flash_buffer, 0, FLASH_LEN, send_buffer + 5);
                send_buffer[5 + FLASH_7BIT_LEN] = 0xF7;
                usb_send(send_buffer, 6 + FLASH_7BIT_LEN);
            } break;

            // Set flash
            case 0x04: {
                bits7_to_bytes(sysex_recv_buffer, 5, FLASH_LEN, flash_buffer);
                hexdump("WRITE", flash_buffer, FLASH_LEN);
                write_flash_data(0, FLASH_LEN, flash_buffer);
                send_buffer[2] = device_id;
                send_buffer[4] = 0x4;
                send_buffer[5] = 0x0;
                send_buffer[6] = 0xF7;
                usb_send(send_buffer, 7);
            } break;
            }
        }
    }
    sysex_recv_length = 0;
}

//////////////////////////////////////////////////////////////////////

void process_midi_packet_in(uint8 length)
{
    sysex_recv_packet_offset = 0;

    while(sysex_recv_packet_offset < length) {

        uint8 cmd = Ep2Buffer[sysex_recv_packet_offset];

        switch(cmd) {

        case mci_sysex_start:
        case mci_sysex_end_3:
            sysex_parse_add(3);
            break;

        case mci_sysex_end_1:
            sysex_parse_add(1);
            break;
        case mci_sysex_end_2:
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
// rotary encoder reader

__code const uint8 encoder_valid_bits[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

__idata uint8 encoder_state = 0;
__idata uint8 encoder_store = 0;

int8 read_encoder()
{
    uint8 a = 0;
    if(!ROTA_BIT) {
        a |= 1;
    }
    if(!ROTB_BIT) {
        a |= 2;
    }

    encoder_state <<= 2;
    encoder_state |= a;
    encoder_state &= 0xf;

    if(encoder_valid_bits[encoder_state] != 0) {
        encoder_store = (encoder_store << 4) | encoder_state;
        switch(encoder_store) {
        case 0xe8:
            return 1;
        case 0x2b:
            return -1;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////

int main()
{
    uint16_t press_time = 0;

    CfgFsys();       // CH559 clock select configuration
    mDelaymS(5);     // Modify the main frequency and wait for the internal crystal stability, it will be added
    UART0_Init();    // Candidate 0, can be used for debugging

    chip_id = CHIP_UNIQUE_ID_LO | ((uint32)CHIP_UNIQUE_ID_HI << 16);

    hexdump("===== CHIPID =====", &chip_id, 4);

    // set GPIO 1.6 as output push/pull

    P1_MOD_OC = 0b00000000;
    P1_DIR_PU = 0b01000000;

    // set GPIO 3.3 as bidirectional (button)
    // set GPIO 3.1 as bidirectional (rx)
    // set GPIO 3.0 as output push/pull (tx)

    P3_MOD_OC = 0b00011110;
    P3_DIR_PU = 0b00011111;

    putstr("Init USB\n");
    usb_device_config();
    usb_device_endpoint_config();    // Endpoint configuration
    usb_device_int_config();         // Interrupt initialization
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;    // Be pre -use and sending length must be empty
    UEP2_T_LEN = 0;    // Be pre -use and sending length must be empty

#define DEBOUNCE_TIME 0xA0u
#define T2_DEBOUNCE (0xFFu - DEBOUNCE_TIME)

    TR0 = 1;
    TR2 = 1;

    uint8_t i = 0;
    while(i < 6) {
        if(TF2 == 1) {
            TL2 = 0;
            TH2 = 0;
            TF2 = 0;
            LED_BIT = !LED_BIT;
            i += 1;
        }
    }

    LED_BIT = 0;

    bool button_state = false;    // for debouncing the button

    if(!read_flash_data(DATA_FLASH_ADDR, 1, &vol_direction)) {
        putstr("Error reading flash data\n");
    } else {
        hexdump("Vol Direction", &vol_direction, 1);
    }

    switch(vol_direction) {
    case 0:
        turn_value = -1;
        break;
    case 2:
        turn_value = 1;
        break;
    default:
        turn_value = ROTARY_DIRECTION - 1;
        break;
    }

    putstr("main loop\n");

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
            if(!queue_full() && !usb_send_update()) {

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
