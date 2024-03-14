#pragma once

extern __xdata uint8 Ep0Buffer[DEFAULT_ENDP0_SIZE];
extern __xdata uint8 Ep1Buffer[DEFAULT_ENDP1_SIZE];
extern __xdata uint8 Ep2Buffer[USB_PACKET_SIZE * 2];
extern __xdata uint8 serial_number_string[SERIAL_STRING_LEN];
extern __xdata uint8 product_string[PRODUCT_NAME_STRING_LEN];
