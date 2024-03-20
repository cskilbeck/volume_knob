#pragma once

//////////////////////////////////////////////////////////////////////
// fnfff - this lives in xram because normal ram is very limited

struct usb_data
{
    // if recv_len[x] != 0 then bytes are available in endpoint x
    uint8 recv_len[4];

    // bitmask of idle endpoints for sending (1 = ep1, 2 = ep2, 4 = ep3)
    volatile uint8 idle;

    // bool - is usb active and connected to host
    volatile bool active;

    // internal usb admin
    uint8 setup_request;
    uint8 setup_len;
    uint8 config;
    uint8 const *current_descriptor;
    uint8 const *current_config_desc;
};

//////////////////////////////////////////////////////////////////////

typedef enum usb_endpoint
{
    endpoint_1 = 1,
    endpoint_2 = 2,
    endpoint_3 = 3,
    endpoint_4 = 4
} usb_endpoint_t;

//////////////////////////////////////////////////////////////////////

void usb_isr(void) __interrupt(INT_NO_USB);

void usb_init_strings();
void usb_device_config();
void usb_device_endpoint_config();
void usb_device_int_config();
void usb_wait_for_connection();
void usb_send(usb_endpoint_t endpoint, uint8 len);
bool usb_is_endpoint_idle(usb_endpoint_t endpoint);
