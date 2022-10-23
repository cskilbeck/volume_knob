//////////////////////////////////////////////////////////////////////

void usb_isr(void) __interrupt(INT_NO_USB);

void usb_init();

// change the state of the key (or 0 to release)

void usb_set_keystate(uint8_t key);

// only call usb_set_keystate when this is zero

extern volatile __idata uint8_t usb_idle;
