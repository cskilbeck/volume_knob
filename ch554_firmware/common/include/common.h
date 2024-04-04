#pragma once

//////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <string.h>

#define bool _Bool
#define true 1
#define false 0

//////////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#include "nanoprintf.h"
#else
#define print(...) NOP_MACRO
#define printf(...) NOP_MACRO
#define puts(...) NOP_MACRO
#endif

//////////////////////////////////////////////////////////////////////

#include "vs_lint.h"
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
