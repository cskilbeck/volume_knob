//////////////////////////////////////////////////////////////////////

#if !defined(XDATA)
#define XDATA extern __xdata
#endif

// System config channel
XDATA uint8 usb_endpoint_0_buffer[DEFAULT_ENDP0_SIZE];

// User endpoint buffers
XDATA uint8 usb_endpoint_1_rx_buffer[USB_PACKET_SIZE];
XDATA uint8 usb_endpoint_1_tx_buffer[USB_PACKET_SIZE];
XDATA uint8 usb_endpoint_2_rx_buffer[USB_PACKET_SIZE];
XDATA uint8 usb_endpoint_2_tx_buffer[USB_PACKET_SIZE];
XDATA uint8 usb_endpoint_3_rx_buffer[USB_PACKET_SIZE];
XDATA uint8 usb_endpoint_3_tx_buffer[USB_PACKET_SIZE];

// serial number stuff
XDATA uint8 serial_number_string[(2 + 2 * SERIAL_LEN)];                     // e.g "012345678"
XDATA uint8 product_name_string[(2 + 2 * ((sizeof(PRODUCT_NAME) - 1)))];    // e.g. "Tiny USB Knob 012345678"

XDATA struct usb_data usb;

#include "xdata_extra.h"

#undef XDATA
