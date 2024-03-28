
// DO NOT PUT #pragma once in here!

#if !defined(XDATA)
#define XDATA extern __xdata
#endif

XDATA uint8 midi_send_buffer[48];
XDATA uint8 midi_recv_buffer[48];
XDATA midi_queue_t midi_queue;
XDATA struct midi_config midi_config;

#undef XDATA
