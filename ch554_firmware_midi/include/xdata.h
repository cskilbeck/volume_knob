#pragma once

#include "ch554_usb.h"

extern __xdata uint8 endpoint_0_buffer[DEFAULT_ENDP0_SIZE];      // endpoint0 OUT & IN buffer，Must be an even address
extern __xdata uint8 endpoint_1_buffer[DEFAULT_ENDP1_SIZE];      // endpoint1 upload buffer
extern __xdata uint8 endpoint_2_in_buffer[MAX_PACKET_SIZE];      // endpoint2 IN buffer, Must be an even address
extern __xdata uint8 endpoint_2_out_buffer[MAX_PACKET_SIZE];     // endpoint2 OUT buffer, Must be an even address and immediately succeed IN buffer
extern __xdata uint8 serial_number_string[SERIAL_STRING_LEN];    // USB serial number from Chip ID
extern __xdata uint8 product_string[PRODUCT_NAME_STRING_LEN];    // Product name + serial number
extern __xdata uint8 midi_send_buffer[48];
extern __xdata uint8 midi_recv_buffer[48];
extern __xdata save_buffer_t save_buffer;
extern __xdata uint8 queue_buffer[MIDI_QUEUE_LEN * MIDI_PACKET_SIZE];
