//////////////////////////////////////////////////////////////////////

#if !defined(XDATA)
#define XDATA extern __xdata
#endif

XDATA uint8 usb_endpoint_0_buffer[DEFAULT_ENDP0_SIZE];     // USB Config channel
XDATA uint8 usb_endpoint_1_buffer[USB_PACKET_SIZE];        // Keyboard
XDATA uint8 usb_endpoint_2_buffer[USB_PACKET_SIZE];        // Media
XDATA uint8 usb_endpoint_3_buffer[USB_PACKET_SIZE * 2];    // Custom

XDATA uint8 serial_number_string[(2 + 2 * SERIAL_LEN)];                     // e.g "012345678"
XDATA uint8 product_name_string[(2 + 2 * ((sizeof(PRODUCT_NAME) - 1)))];    // e.g. "Tiny USB Knob 012345678"

#undef XDATA
