#pragma once

#pragma disable_warning 110
#pragma disable_warning 154

//////////////////////////////////////////////////////////////////////
// device type config

#define DEVICE_ORIGINAL 0
#define DEVICE_DIRECT 1
#define DEVICE_DEVKIT 2

#if !defined(DEVICE)
#error DEVICE is not defined?

// uncomment one of these for VSCode section dimming thing
#define DEVICE DEVICE_DEVKIT
// #define DEVICE DEVICE_DIRECT
// #define DEVICE DEVICE_ORIGINAL
#endif

//////////////////////////////////////////////////////////////////////
// enable uart0 output

#if DEVICE == DEVICE_DEVKIT
// #undef DEBUG
#define DEBUG
#endif

//////////////////////////////////////////////////////////////////////

#define TICK_FREQ 1000

#define UART0_BAUD 115200

#define BOOTLOADER_FLASH_LED_COUNT 10
#define BOOTLOADER_FLASH_LED_SPEED 40

#define BOOT_FLASH_LED_COUNT 20
#define BOOT_FLASH_LED_SPEED 20

#if defined(DEBUG)
#define BOOTLOADER_BUTTON_DELAY_MS 1000
#else
#define BOOTLOADER_BUTTON_DELAY_MS 5000
#endif

//////////////////////////////////////////////////////////////////////
// USB config

// uncomment this line for full speed (12Mb), comment it out for low speed (1.5Mb)
#define USB_FULL_SPEED 1

#if defined(USB_FULL_SPEED)
#define USB_PACKET_SIZE 16
#else
#define USB_PACKET_SIZE 8
#endif

//////////////////////////////////////////////////////////////////////
// USB String descriptor stuff

#define SERIAL_LEN 8    // 8 chars for 32 bit hex serial # from chip_id 01234567

#define PRODUCT_NAME "Tiny Volume Knob 00000000"    // last 8 chars filled in with serial #

//////////////////////////////////////////////////////////////////////

#define NOP_MACRO \
    do {          \
    } while(false)

//////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#include <stdio.h>
#else
#define printf(...) NOP_MACRO
#define puts(...) NOP_MACRO
#endif

//////////////////////////////////////////////////////////////////////

#include "ch554.h"
#include "ch554_usb.h"

//////////////////////////////////////////////////////////////////////

#include "types.h"
#include "xdata.h"
#include "gpio.h"
#include "debug.h"
#include "util.h"
#include "tick.h"
#include "usb.h"
#include "hid_keys.h"
#include "led.h"
#include "encoder.h"
