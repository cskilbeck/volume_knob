#pragma once

//////////////////////////////////////////////////////////////////////
// VID = 16D0, PID = 1317

#define USB_VID 0x16D0
#define USB_PID 0x1317

//////////////////////////////////////////////////////////////////////

extern __xdata __at(0x0000) uint8 Ep0Buffer[DEFAULT_ENDP0_SIZE];     // endpoint0 OUT & IN buffer，Must be an even address
extern __xdata __at(0x0040) uint8 Ep1Buffer[DEFAULT_ENDP1_SIZE];     // endpoint1 upload buffer
extern __xdata __at(0x0080) uint8 Ep2Buffer[2 * MAX_PACKET_SIZE];    // endpoint2 IN & OUT buffer, Must be an even address

extern volatile __idata uint8 ep2_recv_len;    // # received by USB endpoint
extern volatile __idata uint8 ep2_busy;        // upload endpoint busy flag
extern volatile __idata uint8 usb_config;

void usb_device_config();
void usb_device_int_config();
void usb_device_endpoint_config();
void usb_device_irq_handler();

// you must make this declaration visible to the module which contains main() in order
// for SDCC to emit the irq vector table correctly...
void usb_irq_handler(void) __interrupt(INT_NO_USB);
