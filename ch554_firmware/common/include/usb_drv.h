#pragma once

//////////////////////////////////////////////////////////////////////

typedef struct usb_descriptor
{
    uint8 const *p;
    uint8 len;

} usb_descriptor_t;

//////////////////////////////////////////////////////////////////////

typedef enum usb_endpoint
{
    endpoint_1 = 0,
    endpoint_2 = 1,
    endpoint_3 = 2,
    //endpoint_4 = 3,    // endpoint 4 not supported

    num_endpoints = 3

} usb_endpoint_t;

//////////////////////////////////////////////////////////////////////

typedef struct usb_config_data
{
    usb_descriptor_t *device_descriptor;
    usb_descriptor_t *config_descriptors;
    usb_descriptor_t *report_descriptors;
    usb_descriptor_t *string_descriptors;

    uint8 num_config_descriptors;
    uint8 num_report_descriptors;
    uint8 num_string_descriptors;

    char const *product_name;
    uint8 product_name_length;

} usb_config_data_t;

// user code defines this

extern usb_config_data_t const usb_cfg;

//////////////////////////////////////////////////////////////////////
// fnfff - this lives in xram because normal ram is very limited

typedef struct usb_data
{
    // if recv_len[x] != 0 then bytes are available in endpoint x
    uint8 recv_len[num_endpoints];

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

} usb_data_t;

//////////////////////////////////////////////////////////////////////

#define DESCRIPTOR(x)    \
    {                    \
        x, SIZEOF_LSB(x) \
    }

//////////////////////////////////////////////////////////////////////

void usb_init();
void usb_wait_for_connection();
void usb_send(usb_endpoint_t endpoint, uint8 len);
bool usb_is_endpoint_idle(usb_endpoint_t endpoint);
void usb_isr(void) __interrupt(INT_NO_USB);
