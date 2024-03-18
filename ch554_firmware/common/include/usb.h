#pragma once

//////////////////////////////////////////////////////////////////////
// fnfff - this lives in xram because normal ram is very limited

struct usb_data
{
    uint8 setup_request;
    uint8 setup_len;
    uint8 config;
    uint8 recv_len[4];
    uint8 const *current_descriptor;
    volatile uint8 idle;
    volatile uint8 active;
    uint8 const *current_config_desc;
};

//////////////////////////////////////////////////////////////////////

void usb_isr(void) __interrupt(INT_NO_USB);

void usb_init_strings();
void usb_device_config();
void usb_device_endpoint_config();
void usb_device_int_config();
void usb_wait_for_connection();
