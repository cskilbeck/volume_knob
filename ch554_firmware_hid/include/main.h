#pragma once

#pragma disable_warning 110
#pragma disable_warning 154

//////////////////////////////////////////////////////////////////////
// system includes

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "ch554.h"
#include "ch554_usb.h"
#include "types.h"
#include "gpio.h"
#include "debug.h"
#include "util.h"

//////////////////////////////////////////////////////////////////////
// device type config

#define DEVICE_ORIGINAL 0
#define DEVICE_DIRECT 1
#define DEVICE_DEVKIT 2

#if !defined(DEVICE)
// #define DEVICE DEVICE_ORIGINAL
#error DEVICE is not defined?
#endif

//////////////////////////////////////////////////////////////////////
// USB config

// uncomment this line for full speed (12Mb), comment it out for low speed (1.5Mb)

#define USB_FULL_SPEED 1

#if defined(USB_FULL_SPEED)
#define UDEV_LOW_SPEED 0
#define UCTL_LOW_SPEED 0
#define USB_PACKET_SIZE 64
#else
#define UDEV_LOW_SPEED bUD_LOW_SPEED
#define UCTL_LOW_SPEED bUC_LOW_SPEED
#define USB_PACKET_SIZE 8
#endif

//////////////////////////////////////////////////////////////////////

#define SERIAL_LEN 8    // 01234567
#define SERIAL_STRING_LEN (2 + 2 * SERIAL_LEN)

#define PRODUCT_NAME "Tiny Volume Knob "
#define PRODUCT_NAME_LEN 17
#define PRODUCT_NAME_STRING_LEN (2 + 2 * (PRODUCT_NAME_LEN + SERIAL_LEN))

//////////////////////////////////////////////////////////////////////
// Define ROTARY_DIRECTION as CLOCKWISE for one kind of encoders, ANTI_CLOCKWISE for
// the other ones (some are reversed). This sets the default rotation (after first
// flash), reverse by triple-clicking the knob

#define CLOCKWISE 2
#define ANTI_CLOCKWISE 0

#if DEVICE == DEVICE_DEVKIT
#define ROTARY_DIRECTION (ANTI_CLOCKWISE)
#else
#define ROTARY_DIRECTION (CLOCKWISE)
#endif

//////////////////////////////////////////////////////////////////////
// how long a button press goes into bootloader

// #define BOOTLOADER_DELAY 0x300    // about 3 seconds
#define BOOTLOADER_DELAY 0x80    // about 3 seconds

//////////////////////////////////////////////////////////////////////
// GPIO ports and pins

#define PORT1 0x90
#define PORT3 0xB0

#if DEVICE == DEVICE_ORIGINAL

// USB Micro B version

#define ROTA_PORT PORT3
#define ROTA_PIN 1

#define ROTB_PORT PORT3
#define ROTB_PIN 0

#define BTN_PORT PORT3
#define BTN_PIN 2

#define LED_PORT PORT1
#define LED_PIN 4

#define LED_ON 0
#define LED_OFF 1

#elif DEVICE == DEVICE_DIRECT

// USB A version

#define ROTA_PORT PORT3
#define ROTA_PIN 3

#define ROTB_PORT PORT3
#define ROTB_PIN 4

#define BTN_PORT PORT3
#define BTN_PIN 2

#define LED_PORT PORT1
#define LED_PIN 6

#define LED_ON 0
#define LED_OFF 1

#elif DEVICE == DEVICE_DEVKIT

// CH554G Devkit version

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

#define LED_ON 1
#define LED_OFF 0

#endif

SBIT(BTN_BIT, BTN_PORT, BTN_PIN);
SBIT(LED_BIT, LED_PORT, LED_PIN);
SBIT(ROTA_BIT, ROTA_PORT, ROTA_PIN);
SBIT(ROTB_BIT, ROTB_PORT, ROTB_PIN);

#define BOOTLOADER_FLASH_LED_COUNT 20
#define BOOTLOADER_FLASH_LED_SPEED 0x20

#define BOOT_FLASH_LED_COUNT 10
#define BOOT_FLASH_LED_SPEED 0x0

//////////////////////////////////////////////////////////////////////

#define TIMER0_FREQ 1000

#define TIMER0_PERIOD (2000000 / TIMER0_FREQ)

#define TIMER0_LOW ((uint8)(255 - (TIMER0_PERIOD % 256)))
#define TIMER0_HIGH ((uint8)(255 - (TIMER0_PERIOD / 256)))

void timer0_irq_handler(void) __interrupt(INT_NO_TMR0);

// #define BOOTLOADER_BUTTON_DELAY 30000    // about 30 seconds
#define BOOTLOADER_BUTTON_DELAY 1000    // about 1 second

//////////////////////////////////////////////////////////////////////
// other includes

#include "xdata.h"
#include "hid.h"
#include "hid_keys.h"
#include "led.h"
#include "encoder.h"
