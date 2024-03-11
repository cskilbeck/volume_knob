//////////////////////////////////////////////////////////////////////

#pragma once

#include <stdint.h>
#include <string.h>
#include "ch554.h"
#include "ch554_usb.h"

//////////////////////////////////////////////////////////////////////

#define DEVICE_ORIGINAL 0
#define DEVICE_DIRECT 1
#define DEVICE_DEVKIT 2

#if !defined(DEVICE)
#define DEVICE DEVICE_DEVKIT
#endif

//////////////////////////////////////////////////////////////////////

#define SERIAL_LEN 8    // 01234567
#define SERIAL_STRING_LEN (2 + 2 * SERIAL_LEN)

#define PRODUCT_NAME "Tiny Midi Knob "
#define PRODUCT_NAME_LEN 15
#define PRODUCT_NAME_STRING_LEN (2 + 2 * (PRODUCT_NAME_LEN + SERIAL_LEN))

//////////////////////////////////////////////////////////////////////

#include "types.h"
#include "gpio.h"
#include "debug.h"
#include "util.h"
#include "usb.h"
#include "midi.h"
#include "xdata.h"
#include "config.h"
#include "encoder.h"

//////////////////////////////////////////////////////////////////////

#define FIRMWARE_CURRENT_VERSION 0x010000

#define FIRMWARE_VERSION ((FIRMWARE_CURRENT_VERSION << 8) | DEVICE)

//////////////////////////////////////////////////////////////////////

#define BOOTLOADER_FLASH_LED_COUNT 20
#define BOOTLOADER_FLASH_LED_SPEED 0x80

#define BOOT_FLASH_LED_COUNT 10
#define BOOT_FLASH_LED_SPEED 0x40

//////////////////////////////////////////////////////////////////////

#define BOOTLOADER_BUTTON_DELAY 30000    // about 30 seconds
// #define BOOTLOADER_BUTTON_DELAY 0x80

//////////////////////////////////////////////////////////////////////

#define TIMER0_FREQ 1000

#define TIMER0_PERIOD (2000000 / TIMER0_FREQ)

#define TIMER0_LOW ((uint8)(255 - (TIMER0_PERIOD % 256)))
#define TIMER0_HIGH ((uint8)(255 - (TIMER0_PERIOD / 256)))

//////////////////////////////////////////////////////////////////////

void led_set_flash();

void timer0_irq_handler(void) __interrupt(INT_NO_TMR0);

//////////////////////////////////////////////////////////////////////

#define PORT1 0x90
#define PORT3 0xB0

//////////////////////////////////////////////////////////////////////

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

#elif DEVICE == DEVICE_DEVKIT

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

#endif

SBIT(BTN_BIT, BTN_PORT, BTN_PIN);
SBIT(LED_BIT, LED_PORT, LED_PIN);
SBIT(ROTA_BIT, ROTA_PORT, ROTA_PIN);
SBIT(ROTB_BIT, ROTB_PORT, ROTB_PIN);
