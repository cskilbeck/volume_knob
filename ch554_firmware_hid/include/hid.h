#pragma once

//////////////////////////////////////////////////////////////////////

void usb_isr(void) __interrupt(INT_NO_USB);

void usb_init_strings();
void usb_device_config();
void usb_device_endpoint_config();
void usb_device_int_config();
void usb_wait_for_connection();

void usb_set_keystate(uint16_t key);

extern volatile __idata uint8_t usb_idle;
extern volatile __idata uint8_t usb_active;