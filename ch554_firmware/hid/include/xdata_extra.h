// DO NOT PUT #pragma once in here!

#if !defined(XDATA)
#define XDATA extern __xdata
#endif

XDATA hid_queue_t hid_queue;
XDATA struct config config;

#undef XDATA
