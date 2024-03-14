//////////////////////////////////////////////////////////////////////

void usb_isr(void) __interrupt(INT_NO_USB);

void usb_device_config();
void usb_device_endpoint_config();    // Endpoint configuration
void usb_device_int_config();         // Interrupt initialization

void usb_set_keystate(uint16_t key);

// only call usb_set_keystate when (usb_idle & 1) == 0
// only call usb_set_media_keystate when (usb_idle & 2) == 0

extern volatile __idata uint8_t usb_idle;
extern volatile __idata uint8_t usb_active;