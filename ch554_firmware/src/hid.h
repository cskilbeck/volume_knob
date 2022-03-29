//////////////////////////////////////////////////////////////////////

#define KEY_MEDIA_MUTE 0xE2
#define KEY_MEDIA_VOL_UP 0xE9
#define KEY_MEDIA_VOL_DOWN 0xEA

//////////////////////////////////////////////////////////////////////

void usb_isr(void) __interrupt(INT_NO_USB);

void usb_init();

void usb_set_keystate(uint8_t key);

extern volatile __idata uint8_t usb_idle;    // only call usb_set_keystate when this is zero
