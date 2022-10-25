//////////////////////////////////////////////////////////////////////

void usb_isr(void) __interrupt(INT_NO_USB);

void usb_init();

void usb_set_keystate(uint16_t key);

// only call usb_set_keystate when (usb_idle & 1) == 0
// only call usb_set_media_keystate when (usb_idle & 2) == 0

extern volatile __idata uint8_t usb_idle;
