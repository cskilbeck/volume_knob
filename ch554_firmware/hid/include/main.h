#pragma once

#pragma disable_warning 110
#pragma disable_warning 154

//////////////////////////////////////////////////////////////////////

#if !defined(FREQ_SYS)
#define FREQ_SYS 24000000
#endif

//////////////////////////////////////////////////////////////////////

#define USB_PACKET_SIZE 64

//////////////////////////////////////////////////////////////////////

#define FIRMWARE_CURRENT_VERSION 0x02000000lu

#define FIRMWARE_VERSION (FIRMWARE_CURRENT_VERSION | DEVICE)

#include "device.h"
#include "common.h"
#include "hid.h"
#include "xdata_extra.h"
#include "hid_config.h"
