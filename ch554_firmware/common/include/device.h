#pragma once

//////////////////////////////////////////////////////////////////////
// device type config

// USB Micro-B female
#define DEVICE_ORIGINAL 0

// USB A male
#define DEVICE_DIRECT 1

// CH554 Dev board
#define DEVICE_DEVKIT 2

// USB Micro-B female CH554E version
#define DEVICE_ORIGINAL_V2 3

#if !defined(DEVICE)
#error DEVICE is not defined?
#endif

//////////////////////////////////////////////////////////////////////
// config_version byte layout (1st byte of every saved config):
//
//   bits 7..4 = device type nibble
//   bits 3..0 = config format version (0..15)
//
// The nibbles are chosen so every config already shipped conforms with no
// renumbering: MIDI used 0x01..0x0A (type 0x0), HID uses 0x82 (type 0x8).
// When a type's format nibble runs out (>0x0F) allocate a fresh type nibble
// for that device and have the configurator accept both.

#define CONFIG_DEVTYPE_MIDI 0x0
#define CONFIG_DEVTYPE_HID  0x8

#define MAKE_CONFIG_VERSION(devtype, fmt) (((devtype) << 4) | (fmt))
