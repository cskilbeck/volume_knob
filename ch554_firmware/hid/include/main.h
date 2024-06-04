#pragma once

#pragma disable_warning 110
#pragma disable_warning 154

//////////////////////////////////////////////////////////////////////
// device type config

// USB Micro-B female
#define DEVICE_ORIGINAL 0

// USB A male
#define DEVICE_DIRECT 1

// CH554 Dev board
#define DEVICE_DEVKIT 2

#if !defined(DEVICE)
#error DEVICE is not defined?
#endif

//////////////////////////////////////////////////////////////////////

#if !defined(FREQ_SYS)
#define FREQ_SYS 24000000
#endif

//////////////////////////////////////////////////////////////////////

#define USB_PACKET_SIZE 64

//////////////////////////////////////////////////////////////////////

#define FIRMWARE_CURRENT_VERSION 0x01000000lu

#define FIRMWARE_VERSION (FIRMWARE_CURRENT_VERSION | DEVICE)

#include "common.h"

#include "hid.h"
#include "xdata_extra.h"
#include "hid_config.h"
