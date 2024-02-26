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
#include "midi.h"

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
// XDATA, 1KB available

__xdata uint8 Ep0Buffer[DEFAULT_ENDP0_SIZE];     // endpoint0 OUT & IN buffer，Must be an even address
__xdata uint8 Ep1Buffer[DEFAULT_ENDP1_SIZE];     // endpoint1 upload buffer
__xdata uint8 Ep2Buffer[2 * MAX_PACKET_SIZE];    // endpoint2 IN & OUT buffer, Must be an even address
__xdata uint8 midi_send_buffer[48];
__xdata uint8 midi_recv_buffer[48];
__xdata save_buffer_t save_buffer;
__xdata midi_packet queue_buffer[MIDI_QUEUE_LEN];

//////////////////////////////////////////////////////////////////////
// Flash LED before jumping to bootloader

void led_flash(int8_t n, uint8 speed)
{
    LED_BIT = 1;
    for(int8_t i = 0; i < n; ++i) {
        TF2 = 0;
        TH2 = speed;
        TL2 = 0;
        while(TF2 != 1) {
        }
        LED_BIT ^= 1;
    }
    LED_BIT = 0;
}

//////////////////////////////////////////////////////////////////////

int main()
{
    CfgFsys();       // CH559 clock select configuration
    mDelaymS(5);     // Modify the main frequency and wait for the internal crystal stability, it will be added
    UART0_Init();    // Candidate 0, can be used for debugging

    init_chip_id();

    hexdump("===== CHIPID =====", &chip_id, 4);

    gpio_init(UART_TX_PORT, UART_TX_PIN, gpio_output_push_pull);
    gpio_init(UART_RX_PORT, UART_RX_PIN, gpio_output_open_drain);
    gpio_init(ROTA_PORT, ROTA_PIN, gpio_input_pullup);
    gpio_init(ROTB_PORT, ROTB_PIN, gpio_input_pullup);
    gpio_init(BTN_PORT, BTN_PIN, gpio_input_pullup);
    gpio_init(LED_PORT, LED_PIN, gpio_output_push_pull);

    // init usb
    usb_device_config();
    usb_device_endpoint_config();    // Endpoint configuration
    usb_device_int_config();         // Interrupt initialization
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;    // Be pre -use and sending length must be empty
    UEP2_T_LEN = 0;    // Be pre -use and sending length must be empty

    TR0 = 1;
    TR2 = 1;

    led_flash(20, 0x80);

    uint16_t press_time = 0;
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

                    led_flash(10, 0x40);

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
