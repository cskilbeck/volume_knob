#include "main.h"

//////////////////////////////////////////////////////////////////////

#define MIN(x, y) ((x) < (y) ? (x) : (y))

//////////////////////////////////////////////////////////////////////

#define VENDOR_ID 0x16D0
#define PRODUCT_ID 0x114B

//////////////////////////////////////////////////////////////////////

#if defined(USB_FULL_SPEED)
#define UDEV_LOW_SPEED 0
#define UCTL_LOW_SPEED 0
#else
#define UDEV_LOW_SPEED bUD_LOW_SPEED
#define UCTL_LOW_SPEED bUC_LOW_SPEED
#endif

//////////////////////////////////////////////////////////////////////

typedef struct descriptor
{
    uint8 const *p;
    uint8 len;
} descriptor_t;

//////////////////////////////////////////////////////////////////////

#define usb_setup ((USB_SETUP_REQ *)Ep0Buffer)

//////////////////////////////////////////////////////////////////////

__idata uint8 setup_request;
__idata uint8 setup_len;
__idata uint8 usb_config;

uint8 const *descriptor;

volatile __idata uint8 usb_idle   = 3;
volatile __idata uint8 usb_active = 0;

//////////////////////////////////////////////////////////////////////

// uint8 media_key_report[3] = {
//     0x2,    //  8 bits: report ID 2
//     0x0,    // 16 bits: media key (0..7)
//     0x0     // 16 bits: media key (8..15)
// };

// uint8 keyboard_report[8] = {
//     0x00,    // modifier keys
//     0x00,    // pad
//     0x00,    // key 0
//     0x00,    // key 1
//     0x00,    // key 2
//     0x00,    // key 3
//     0x00,    // key 4
//     0x00     // key 5
// };

//////////////////////////////////////////////////////////////////////

// clang-format off

// KEYBOARD DEVICE

__code const uint8 keyboard_rep_desc[] = {

    0x05, 0x01,          // Usage Page: Generic Desktop Controls
    0x09, 0x06,          // Usage: Keyboard
    0xA1, 0x01,          // Collection: Application
    0x05, 0x07,          // Usage Page: Keyboard
    0x19, 0xE0,          // Usage Minimum: Keyboard LeftControl
    0x29, 0xE7,          // Usage Maximum: Keyboard Right GUI
    0x15, 0x00,          // Logical Minimum: 0
    0x25, 0x01,          // Logical Maximum: 1
    0x75, 0x01,          // Report Size: 1
    0x95, 0x08,          // Report Count: 8
    0x81, 0x02,          // Input: Data (2)
    0x95, 0x01,          // Report Count: 1
    0x75, 0x08,          // Report Size: 8
    0x81, 0x01,          // Input: Constant (1)
    0x95, 0x03,          // Report Count: 3
    0x75, 0x01,          // Report Size: 1
    0x05, 0x08,          // Usage Page: LEDs
    0x19, 0x01,          // Usage Minimum: Num Lock
    0x29, 0x03,          // Usage Maximum: Scroll Lock
    0x91, 0x02,          // Output: Data (2)
    0x95, 0x05,          // Report Count: 5
    0x75, 0x01,          // Report Size: 1
    0x91, 0x01,          // Output: Constant (1)
    0x95, 0x06,          // Report Count: 6
    0x75, 0x08,          // Report Size: 8
    0x15, 0x00,          // Logical Minimum: 0
    0x26, 0xFF, 0x00,    // Logical Maximum: 255
    0x05, 0x07,          // Usage Page: Keyboard/Keypad
    0x19, 0x00,          // Usage Minimum: 0
    0x2A, 0xFF, 0x00,    // Usage Maximum: 255
    0x81, 0x00,          // Input: Data (0)
    0xC0                 // End collection
};

// CONSUMER CONTROL DEVICE

__code const uint8 media_rep_desc[] = {

    0x05, 0x0c,                      // USAGE_PAGE (Consumer Devices)
    0x0b, 0x01, 0x00, 0x0c, 0x00,    // USAGE (Consumer Devices:Consumer Control)
    0xa1, 0x01,                      // COLLECTION (Application)
    0x85, 0x02,                      //   REPORT_ID (2)
    0x19, 0x00,                      //   USAGE_MINIMUM (Unassigned)
    0x2a, 0x08, 0x01,                //   USAGE_MAXIMUM (Police Alarm)
    0x15, 0x00,                      //   LOGICAL_MINIMUM (0)
    0x26, 0x3c, 0x02,                //   LOGICAL_MAXIMUM (572)
    0x95, 0x01,                      //   REPORT_COUNT (1)
    0x75, 0x10,                      //   REPORT_SIZE (16)
    0x81, 0x00,                      //   INPUT (Data,Array,Absolute)
    0xc0                             // END_COLLECTION
};

// clang-format on

//////////////////////////////////////////////////////////////////////

__code const uint8 device_desc[] = {

    sizeof(device_desc),     // bLength
    USB_DESCR_TYP_DEVICE,    // bDescriptorType
    0x10,                    // bcdUSB (1)
    0x01,                    // bcdUSB (2)
    0x00,                    // bDeviceClass
    0x00,                    // bDeviceSubClass
    0x00,                    // bDeviceProtocol
    DEFAULT_ENDP0_SIZE,      // bMaxPacketSize0
    VENDOR_ID & 0xff,        // idVendor (LSB)
    VENDOR_ID >> 8,          // idVendor (MSB)
    PRODUCT_ID & 0xff,       // idProduct (LSB)
    PRODUCT_ID >> 8,         // idProduct (MSB)
    0x00,                    // bcdDevice(1)
    0x01,                    // bcdDevice(2)
    0x01,                    // iManufacturer
    0x02,                    // iProduct
    0x03,                    // iSerialNumber
    0x01                     // bNumConfigurations
};

//////////////////////////////////////////////////////////////////////

__code const uint8 hid_config_desc[] = {

    // Config
    0x09,                              // bLength
    USB_DESCR_TYP_CONFIG,              // bDescriptorType
    sizeof(hid_config_desc) & 0xff,    // wTotalLength (1)
    sizeof(hid_config_desc) >> 8,      // wTotalLength (2)
    0x02,                              // bNumInterface
    0x01,                              // bConfigurationValue
    0x06,                              // iConfiguration
    0x80,                              // bmAttributes: Bus Power/No Remote Wakeup
    0x32,                              // bMaxPower

    // Interface
    0x09,                    // bLength
    USB_DESCR_TYP_INTERF,    // bDescriptorType
    0x00,                    // bInterfaceNumber
    0x00,                    // bAlternateSetting
    0x01,                    // bNumEndpoints
    USB_DEV_CLASS_HID,       // bInterfaceClass
    0x01,                    // bInterfaceSubClass
    0x01,                    // bInterfaceProtocol: Keyboard
    0x04,                    // iInterface

    // HID
    0x09,                                // bLength
    USB_DESCR_TYP_HID,                   // bDescriptorType: HID
    0x11,                                // bcdHID(1)
    0x01,                                // bcdHID(2)
    0x00,                                // bCountryCode
    0x01,                                // bNumDescriptors
    0x22,                                // bDescriptorType: Report
    sizeof(keyboard_rep_desc) & 0xff,    // wDescriptorLength (1)
    sizeof(keyboard_rep_desc) >> 8,      // wDescriptorLength (2)

    // Endpoint
    0x07,                      // bLength
    USB_DESCR_TYP_ENDP,        // bDescriptorType: ENDPOINT
    0x81,                      // bEndpointAddress: IN/Endpoint1
    USB_ENDP_TYPE_INTER,       // bmAttributes: Interrupt
    USB_PACKET_SIZE & 0xff,    // wMaxPacketSize (1)
    USB_PACKET_SIZE >> 8,      // wMaxPacketSize (2)
    0x02,                      // bInterval

    // Interface
    0x09,                    // bLength
    USB_DESCR_TYP_INTERF,    // bDescriptorType
    0x01,                    // bInterfaceNumber
    0x00,                    // bAlternateSetting
    0x01,                    // bNumEndpoints
    USB_DEV_CLASS_HID,       // bInterfaceClass
    0x01,                    // bInterfaceSubClass
    0x01,                    // bInterfaceProtocol: Keyboard
    0x05,                    // iInterface

    // HID
    0x09,                             // bLength
    USB_DESCR_TYP_HID,                // bDescriptorType: HID
    0x11,                             // bcdHID(1)
    0x01,                             // bcdHID(2)
    0x00,                             // bCountryCode
    0x01,                             // bNumDescriptors
    0x22,                             // bDescriptorType: Report
    sizeof(media_rep_desc) & 0xff,    // wDescriptorLength (1)
    sizeof(media_rep_desc) >> 8,      // wDescriptorLength (2)

    // Endpoint
    0x07,                      // bLength
    USB_DESCR_TYP_ENDP,        // bDescriptorType: ENDPOINT
    0x82,                      // bEndpointAddress: IN/Endpoint2
    USB_ENDP_TYPE_INTER,       // bmAttributes: Interrupt
    USB_PACKET_SIZE & 0xff,    // wMaxPacketSize (1)
    USB_PACKET_SIZE >> 8,      // wMaxPacketSize (2)
    0x02                       // bInterval
};

//////////////////////////////////////////////////////////////////////
// UINT16s in here...

#define LANGUAGE_DESC 0x0409
#define MANUFACTURER_STRING 'T', 'i', 'n', 'y', ' ', 'L', 'i', 't', 't', 'l', 'e', ' ', 'T', 'h', 'i', 'n', 'g', 's'
#define KEYBOARD_STRING 'K', 'b', 'd'
#define MEDIA_STRING 'M', 'e', 'd', 'i', 'a'
#define CONFIG_STRING 'C', 'o', 'n', 'f', 'i', 'g'

#define STR_HDR(x) (sizeof(x) | (USB_DESCR_TYP_STRING << 8))

__code const uint16 language_desc[]       = { STR_HDR(language_desc), LANGUAGE_DESC };
__code const uint16 manufacturer_string[] = { STR_HDR(manufacturer_string), MANUFACTURER_STRING };
__code const uint16 keyboard_string[]     = { STR_HDR(keyboard_string), KEYBOARD_STRING };
__code const uint16 media_string[]        = { STR_HDR(media_string), MEDIA_STRING };
__code const uint16 config_string[]       = { STR_HDR(config_string), CONFIG_STRING };

//////////////////////////////////////////////////////////////////////

#define DESCRIPTOR(x) \
    {                 \
        x, sizeof(x)  \
    }

//////////////////////////////////////////////////////////////////////

__code const descriptor_t config_descs[] = {
    DESCRIPTOR(hid_config_desc)    // USB config
};

//////////////////////////////////////////////////////////////////////

__code const descriptor_t report_descs[] = {
    DESCRIPTOR(keyboard_rep_desc),    // keyboard HID report
    DESCRIPTOR(media_rep_desc)        // media key HID report
};

//////////////////////////////////////////////////////////////////////

__code const descriptor_t string_descs[] = {
    DESCRIPTOR(language_desc),           // 0
    DESCRIPTOR(manufacturer_string),     // 1
    DESCRIPTOR(product_name_string),     // 2 - dynamic in xdata
    DESCRIPTOR(serial_number_string),    // 3 - dynamic in xdata
    DESCRIPTOR(keyboard_string),         // 4
    DESCRIPTOR(media_string),            // 5
    DESCRIPTOR(config_string)            // 6
};

//////////////////////////////////////////////////////////////////////

#define NUM_CONFIG_DESCS COUNTOF(config_descs)
#define NUM_REPORT_DESCS COUNTOF(report_descs)
#define NUM_STRING_DESCS COUNTOF(string_descs)

//////////////////////////////////////////////////////////////////////

void usb_isr(void) __interrupt(INT_NO_USB)
{
    uint8 len;

    if(UIF_TRANSFER) {

        switch(USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) {

            // [??] input (from us to host) for interface 1,2 arrived, we can send
            // more

        case UIS_TOKEN_IN | 1:
            UEP1_T_LEN = 0;
            UEP1_CTRL  = (UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            usb_idle |= 1;
            break;

        case UIS_TOKEN_IN | 2:
            UEP2_T_LEN = 0;
            UEP2_CTRL  = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            usb_idle |= 2;
            break;

            // setup request on interface 0

        case UIS_TOKEN_SETUP | 0:

            len = 0xff;

            // should be this much, anything else seems to be an error
            if(USB_RX_LEN == (sizeof(USB_SETUP_REQ))) {

                // how much it wants, capped at 127
                setup_len = usb_setup->wLengthL;
                if((usb_setup->wLengthH != 0) || setup_len > 0x7f) {
                    setup_len = 0x7f;
                }

                len                = 0;
                setup_request      = usb_setup->bRequest;
                uint8 request_type = usb_setup->bRequestType;

                // all request types except 'standard' are ignored
                switch(request_type & USB_REQ_TYP_MASK) {

                case USB_REQ_TYP_STANDARD:

                    switch(setup_request) {

                    case USB_GET_DESCRIPTOR:

                        switch(usb_setup->wValueH) {

                        case USB_DESCR_TYP_DEVICE:
                            descriptor = device_desc;
                            len        = sizeof(device_desc);
                            break;

                        case USB_DESCR_TYP_CONFIG: {
                            uint8 desc = usb_setup->wValueL;
                            if(desc < NUM_CONFIG_DESCS) {
                                descriptor = config_descs[desc].p;
                                len        = config_descs[desc].len;
                            } else {
                                len = 0xff;
                            }
                        } break;

                        case USB_DESCR_TYP_REPORT: {
                            uint8 desc = usb_setup->wIndexL;    // !! Index!? I guess...
                            if(desc < NUM_REPORT_DESCS) {
                                descriptor = report_descs[desc].p;
                                len        = report_descs[desc].len;
                            } else {
                                len = 0xff;
                            }
                        } break;

                        case USB_DESCR_TYP_STRING: {
                            uint8 desc = usb_setup->wValueL;
                            if(desc < NUM_STRING_DESCS) {
                                descriptor = string_descs[desc].p;
                                len        = string_descs[desc].len;
                            } else {
                                len = 0xff;
                            }
                        } break;

                        default:
                            len = 0xff;
                            break;
                        }

                        // continuation?
                        if(setup_len > len) {
                            setup_len = len;
                        }

                        // send at most ? bytes per request (coalesced on host)
                        len = MIN(setup_len, DEFAULT_ENDP0_SIZE);
                        memcpy(Ep0Buffer, descriptor, len);
                        setup_len -= len;
                        descriptor += len;
                        break;

                    case USB_SET_ADDRESS:
                        puts("SetAddr");
                        setup_len = usb_setup->wValueL;
                        break;

                    case USB_GET_CONFIGURATION:
                        puts("GetConfig");
                        Ep0Buffer[0] = usb_config;
                        setup_len    = MIN(setup_len, 1);
                        break;

                    case USB_SET_CONFIGURATION:
                        puts("SetConfig");
                        usb_config = usb_setup->wValueL;
                        usb_active = 1;
                        break;

                    case USB_GET_INTERFACE:
                        puts("GetInterface...?");
                        break;

                    case USB_CLEAR_FEATURE:

                        switch(usb_setup->bRequestType & 0x1F) {

                        case USB_REQ_RECIP_DEVICE:    // Device

                            if((((uint16)usb_setup->wValueH << 8) | usb_setup->wValueL) == 0x01) {
                                if((hid_config_desc[7] & 0x20) != 0) {
                                    puts("Wake");
                                } else {
                                    puts("Wake not supported");
                                    len = 0xFF;    // operation failed
                                }
                            } else {
                                len = 0xFF;    // operation failed
                            }
                            break;

                        case USB_REQ_RECIP_ENDP:    // Endpoint

                            switch(usb_setup->wIndexL) {
                            case 0x83:
                                UEP3_CTRL = UEP3_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                break;
                            case 0x03:
                                UEP3_CTRL = UEP3_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                                break;
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                break;
                            case 0x01:
                                UEP1_CTRL = UEP1_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                                break;
                            default:
                                len = 0xFF;    // unknown endpoint
                                break;
                            }
                            break;

                        default:
                            len = 0xFF;    // unsupported
                            break;
                        }

                    case USB_SET_FEATURE:

                        switch(request_type & USB_REQ_RECIP_MASK) {

                        case USB_REQ_RECIP_DEVICE: {
                            if(*(uint16_t *)&usb_setup->wValueL == 0x0001) {

                                // asking to set remote wakeup!?
                                if((hid_config_desc[7] & 0x20) != 0) {
                                    // len = 0;
                                } else {
                                    len = 0xff;
                                }
                            } else {
                                len = 0xff;
                            }
                        } break;

                        case USB_REQ_RECIP_ENDP: {

                            if(*(uint16_t *)&usb_setup->wValueL == 0x0000) {
                                switch(*(uint16_t *)&usb_setup->wIndexL) {
                                case 0x83:
                                    UEP3_CTRL = UEP3_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;    // Set endpoint 3 IN STALL
                                    break;
                                case 0x03:
                                    UEP3_CTRL = UEP3_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;    // Set endpoint 3 OUT Stall
                                    break;
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;    // Set endpoint 2 IN STALL
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;    // Set endpoint 2 OUT Stall
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;    // Set endpoint 1 IN STALL
                                    break;
                                case 0x01:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;    // Set endpoint 1 OUT Stall
                                default:
                                    len = 0xFF;    // operation failed
                                    break;
                                }
                            } else {
                                len = 0xff;
                            }
                        } break;

                        default:
                            len = 0xff;
                            break;
                        }
                        break;

                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;    // bus powered
                        Ep0Buffer[1] = 0x00;    // no remote wakeup

                        len = MIN(setup_len, 2);
                        break;

                    default:
                        len = 0xff;
                        break;
                    }
                    break;

                default:

                    switch(setup_request) {

                    case HID_GET_REPORT:
                    case HID_GET_IDLE:
                    case HID_GET_PROTOCOL:
                    case HID_SET_REPORT:
                    case HID_SET_IDLE:
                    case HID_SET_PROTOCOL:
                        len = 0;
                        break;
                    default:
                        len = 0xff;
                        break;
                    }
                    break;
                }
            }

            // if len == 0xff, some error has occurred
            // set the data toggles to 1
            // and indicate stall
            if(len == 0xff) {
                setup_request = 0xff;
                UEP0_CTRL     = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;

            }
            // if len <= 8, some valid data is sitting in EP0Buffer
            // so send it (also set the toggles!?)
            // although it could be zero bytes, in which case we just ack it
            else if(len <= DEFAULT_ENDP0_SIZE) {
                UEP0_T_LEN = len;
                UEP0_CTRL  = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
            }
            break;

            // endpoint0 IN

        case UIS_TOKEN_IN | 0:

            switch(setup_request) {

            case USB_GET_DESCRIPTOR:
                len = MIN(setup_len, 8);
                memcpy(Ep0Buffer, descriptor, len);
                setup_len -= len;
                descriptor += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;
                break;

            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | setup_len;
                UEP0_CTRL  = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;

            default:
                UEP0_T_LEN = 0;
                UEP0_CTRL  = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;

            // endpoint 0 OUT

        case UIS_TOKEN_OUT | 0:
            UEP0_T_LEN = 0;
            UEP0_CTRL  = UEP_R_RES_ACK | UEP_T_RES_ACK;
            break;

        default:
            break;
        }
        UIF_TRANSFER = 0;
    }

    // bus reset
    if(UIF_BUS_RST) {
        puts("reset");
        UEP0_CTRL    = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL    = bUEP_AUTO_TOG | UEP_T_RES_NAK;
        UEP2_CTRL    = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
        USB_DEV_AD   = 0x00;
        UIF_SUSPEND  = 0;
        UIF_TRANSFER = 0;
        usb_config   = 0;
        usb_idle     = 3;
    }

    // USB bus hangs/Wake up
    if(UIF_SUSPEND) {
        UIF_SUSPEND = 0;
        // Hang up
        if(USB_MIS_ST & bUMS_SUSPEND) {
            puts("suspend");
            while(XBUS_AUX & bUART0_TX) {    // Wait for msg to send
            }
            SAFE_MOD  = 0x55;
            SAFE_MOD  = 0xAA;
            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;    // USB or RXD0/1 can be awakened when there is a signal
            PCON |= PD;                                               // Sleep
            SAFE_MOD  = 0x55;
            SAFE_MOD  = 0xAA;
            WAKE_CTRL = 0x00;
        }
    }
    USB_INT_FG = 0x1F;    // clear all writeable USB IRQ flags
}

//////////////////////////////////////////////////////////////////////
// top bit specifies whether it's a media key (1) or normal (0)

#define IS_MEDIA_KEY(x) ((x & 0x8000) != 0)

void usb_set_keystate(uint16_t key)
{
    if(IS_MEDIA_KEY(key)) {
        key &= 0x7fff;
        Ep2Buffer[0] = 0x02;    // REPORT ID
        Ep2Buffer[1] = key & 0xff;
        Ep2Buffer[2] = key >> 8;
        usb_idle &= ~2;
        UEP2_CTRL  = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
        UEP2_T_LEN = 3;
    } else {
        Ep1Buffer[0] = 0x00;    // modifier
        Ep1Buffer[1] = 0x00;
        Ep1Buffer[2] = key;
        Ep1Buffer[3] = 0x00;
        Ep1Buffer[4] = 0x00;
        Ep1Buffer[5] = 0x00;
        Ep1Buffer[6] = 0x00;
        Ep1Buffer[7] = 0x00;
        usb_idle &= ~1;
        UEP1_CTRL  = (UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
        UEP1_T_LEN = 8;
    }
}

//////////////////////////////////////////////////////////////////////

void usb_device_config()
{
    USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;    // The USB device and internal pull-up are enabled, and automatically
                                                             // return to NAK before the interrupt flag is cleared during the interrupt.

    USB_DEV_AD = 0x00;    // device address 0 before host assigns it

#if !defined(USB_FULL_SPEED)
    USB_CTRL |= bUC_LOW_SPEED;
    UDEV_CTRL |= bUD_LOW_SPEED;
#else
    UDEV_CTRL &= ~bUD_LOW_SPEED;    // full speed 12Mbit mode
#endif

    UDEV_CTRL |= bUD_PD_DIS | bUD_PORT_EN;    // Disable DP/DM pull-downs, enable USB port
}

//////////////////////////////////////////////////////////////////////

void usb_device_int_config()
{
    USB_INT_EN |= bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;    // suspend, transmission complete, bus reset IRQs enabled

    USB_INT_FG |= 0x1F;    // Clear outstanding interrupts

    IE_USB = 1;    // Enable USB interrupt
}

//////////////////////////////////////////////////////////////////////

void usb_device_endpoint_config()
{
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;
    UEP2_T_LEN = 0;

    UEP0_DMA = (uint16)Ep0Buffer;
    UEP1_DMA = (uint16)Ep1Buffer;
    UEP2_DMA = (uint16)Ep2Buffer;

    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;

    UEP4_1_MOD = ~(bUEP4_RX_EN | bUEP4_TX_EN | bUEP1_RX_EN | bUEP1_BUF_MOD) | bUEP1_TX_EN;
    UEP2_3_MOD = ~(bUEP3_RX_EN | bUEP3_TX_EN | bUEP3_BUF_MOD | bUEP2_RX_EN | bUEP2_BUF_MOD) | bUEP2_TX_EN;
}

static __code uint8 const product_name[] = PRODUCT_NAME;

void usb_init_strings()
{
#define PRODUCT_PREFIX (sizeof(product_name) - 1 - SERIAL_LEN)

    // insert chip id into serial string and product name string

    product_name_string[0] = sizeof(product_name_string);
    product_name_string[1] = USB_DESCR_TYP_STRING;

    for(uint8 i = 0; i < PRODUCT_PREFIX; ++i) {
        product_name_string[2 + i * 2] = product_name[i];
        product_name_string[3 + i * 2] = 0;
    }

    serial_number_string[0] = sizeof(serial_number_string);
    serial_number_string[1] = USB_DESCR_TYP_STRING;

    uint32 n = chip_id;

    for(uint8 i = 0; i < SERIAL_LEN; ++i) {

        uint8 c = (uint8)(n >> 28);    // @hardcoded

        if(c < 10) {
            c += '0';
        } else {
            c += 'A' - 10;
        }

        serial_number_string[2 + 2 * i] = c;
        serial_number_string[3 + 2 * i] = 0;

        product_name_string[2 + 2 * (i + PRODUCT_PREFIX)] = c;
        product_name_string[3 + 2 * (i + PRODUCT_PREFIX)] = 0;

        n <<= 4;
    }
}

//////////////////////////////////////////////////////////////////////
// flash led slowly until USB is connected (in case of power-only cable)

void usb_wait_for_connection()
{
    LED_BIT = LED_OFF;

    uint16_t flash = 0;

    while(!usb_active) {

        tick_wait();

        flash += 1;
        if(flash == 1000) {
            flash = 0;
            LED_BIT ^= 1;
        }
    }
}
