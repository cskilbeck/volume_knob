#pragma once

#include "ch554_usb.h"

extern __xdata uint8 Ep0Buffer[DEFAULT_ENDP0_SIZE];     // endpoint0 OUT & IN buffer，Must be an even address
extern __xdata uint8 Ep1Buffer[DEFAULT_ENDP1_SIZE];     // endpoint1 upload buffer
extern __xdata uint8 Ep2Buffer[2 * MAX_PACKET_SIZE];    // endpoint2 IN & OUT buffer, Must be an even address
extern __xdata uint8 midi_send_buffer[48];
extern __xdata uint8 midi_recv_buffer[48];
extern __xdata save_buffer_t save_buffer;
extern __xdata midi_packet queue_buffer[MIDI_QUEUE_LEN];
