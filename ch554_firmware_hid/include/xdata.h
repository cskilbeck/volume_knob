#if !defined(XDATA)
#define XDATA extern __xdata
#endif

XDATA uint8 Ep0Buffer[DEFAULT_ENDP0_SIZE];              // USB Config channel
XDATA uint8 Ep1Buffer[USB_PACKET_SIZE];                 // Media
XDATA uint8 Ep2Buffer[USB_PACKET_SIZE];                 // Keyboard
XDATA uint8 serial_number_string[SERIAL_STRING_LEN];    // e.g "012345678"
XDATA uint8 product_string[PRODUCT_NAME_STRING_LEN];    // e.g. "Tiny Midi Knob 012345678"
