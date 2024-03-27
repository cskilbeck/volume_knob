// DO NOT PUT #pragma once in here!

//////////////////////////////////////////////////////////////////////

#if !defined(XDATA)
#define XDATA extern __xdata
#endif

XDATA uint8 usb_endpoint_0_buffer[USB_PACKET_SIZE];

XDATA uint8 usb_endpoint_1_rx_buffer[USB_PACKET_SIZE];
XDATA uint8 usb_endpoint_1_tx_buffer[USB_PACKET_SIZE];
XDATA uint8 usb_endpoint_2_rx_buffer[USB_PACKET_SIZE];
XDATA uint8 usb_endpoint_2_tx_buffer[USB_PACKET_SIZE];
XDATA uint8 usb_endpoint_3_rx_buffer[USB_PACKET_SIZE];
XDATA uint8 usb_endpoint_3_tx_buffer[USB_PACKET_SIZE];

// serial number stuff - see usb_init_strings()
XDATA uint8 serial_number_string[(2 + 2 * SERIAL_LEN)];                     // e.g "012345678"
XDATA uint8 product_name_string[(2 + 2 * ((sizeof(PRODUCT_NAME) - 1)))];    // e.g. "Tiny USB Knob 012345678"

XDATA struct usb_data usb;

XDATA struct save_buffer save_buffer;

#undef XDATA
