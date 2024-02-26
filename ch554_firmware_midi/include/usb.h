#pragma once

//////////////////////////////////////////////////////////////////////
// VID = 16D0, PID = 1317

#define USB_VID 0x16D0
#define USB_PID 0x1317

//////////////////////////////////////////////////////////////////////

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
