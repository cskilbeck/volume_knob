// DO NOT PUT #pragma once in here!

#if !defined(XDATA)
#define XDATA extern __xdata
#endif

XDATA hid_queue_t hid_queue;
XDATA struct hid_config hid_config;

#undef XDATA
