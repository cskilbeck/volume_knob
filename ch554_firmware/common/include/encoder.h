#pragma once

//////////////////////////////////////////////////////////////////////

#if DEVICE == DEVICE_DEVKIT

#define ROTATE_CW -1
#define ROTATE_CCW 1

#elif DEVICE == DEVICE_DIRECT

#define ROTATE_CW 1
#define ROTATE_CCW -1

#elif DEVICE == DEVICE_ORIGINAL

#define ROTATE_CW 1
#define ROTATE_CCW -1

#elif DEVICE == DEVICE_ORIGINAL_V2

#define ROTATE_CW 1
#define ROTATE_CCW -1

#else

#error NO DEVICE DEFINED

#endif

//////////////////////////////////////////////////////////////////////

void encoder_init();
int8 encoder_read();