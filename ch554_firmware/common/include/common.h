#pragma once

//////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////

#if !defined(VSCODE)
#define STATIC_ASSERT _Static_assert
#define SIZEOF_LSB(x) (sizeof(x) & 0xff)
#define SIZEOF_MSB(x) (sizeof(x) >> 8)
#endif

//////////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#include "nanoprintf.h"
#else
#define printf(...) NOP_MACRO(__VA_ARGS__)
#define puts(...) NOP_MACRO(__VA_ARGS__)
#endif

//////////////////////////////////////////////////////////////////////

#include "ch554.h"
#include "ch554_usb.h"
#include "types.h"
#include "debug.h"

#include "process_drv.h"
#include "util.h"
#include "xdata_common.h"
#include "gpio_drv.h"
#include "tick.h"
#include "usb_drv.h"
#include "led.h"
#include "encoder.h"
#include "flash.h"
#include "queue.h"
