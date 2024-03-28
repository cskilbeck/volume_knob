//////////////////////////////////////////////////////////////////////

#pragma once

#pragma disable_warning 110
#pragma disable_warning 154

//////////////////////////////////////////////////////////////////////

#define DEVICE_ORIGINAL 0
#define DEVICE_DIRECT 1
#define DEVICE_DEVKIT 2

#if !defined(DEVICE)
#error Please define DEVICE

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

#define BOOTLOADER_FLASH_LED_COUNT 30
#define BOOTLOADER_FLASH_LED_SPEED 40

#define BOOT_FLASH_LED_COUNT 10
#define BOOT_FLASH_LED_SPEED 20

//////////////////////////////////////////////////////////////////////

// uncomment this line for full speed (12Mb), comment it out for low speed (1.5Mb)
#define USB_FULL_SPEED 1

#if defined(USB_FULL_SPEED)
#define USB_PACKET_SIZE 64
#else
#define USB_PACKET_SIZE 8
#endif

//////////////////////////////////////////////////////////////////////

#define SERIAL_LEN 8    // 01234567

#define PRODUCT_NAME "Tiny Midi Knob 00000000"

#define FIRMWARE_CURRENT_VERSION 0x010000

#define FIRMWARE_VERSION ((FIRMWARE_CURRENT_VERSION << 8) | DEVICE)

//////////////////////////////////////////////////////////////////////
// MIDI_QUEUE_LEN must be a power of 2 and
// MIDI_QUEUE_LEN * sizeof(uint32) must be < USB_PACKET_SIZE (64) so
// effectively the max length is 16

#include "common.h"

#include "midi.h"
#include "xdata_extra.h"
#include "midi_config.h"
