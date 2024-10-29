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
