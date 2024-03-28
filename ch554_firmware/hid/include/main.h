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
#endif

//////////////////////////////////////////////////////////////////////

#if !defined(FREQ_SYS)
#define FREQ_SYS 24000000
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

#define BOOTLOADER_FLASH_LED_COUNT 30
#define BOOTLOADER_FLASH_LED_SPEED 40

#define BOOT_FLASH_LED_COUNT 10
#define BOOT_FLASH_LED_SPEED 20

//////////////////////////////////////////////////////////////////////

#define USB_PACKET_SIZE 64

//////////////////////////////////////////////////////////////////////
// USB String descriptor stuff

#define SERIAL_LEN 8    // 8 chars for 32 bit hex serial # from chip_id 01234567

#define FIRMWARE_CURRENT_VERSION 0x01000000lu

#define FIRMWARE_VERSION (FIRMWARE_CURRENT_VERSION | DEVICE)

#include "common.h"

#include "hid.h"
#include "xdata_extra.h"
#include "config.h"
