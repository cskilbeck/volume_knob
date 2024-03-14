#include "main.h"

//////////////////////////////////////////////////////////////////////

#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define COUNTOF(x) (sizeof(x) / sizeof((x)[0]))

//////////////////////////////////////////////////////////////////////

#define VENDOR_ID 0xD0, 0x16     // VID = 16D0
#define PRODUCT_ID 0x4B, 0x11    // PID = 114B

//////////////////////////////////////////////////////////////////////

#define LANGUAGE_DESCRIPTION 0x09, 0x04

#define MANUFACTURER_DESCRIPTION 'C', 0, 'H', 0, ' ', 0, 'S', 0, 'k', 0, 'i', 0, 'l', 0, 'b', 0, 'e', 0, 'c', 0, 'k', 0

//////////////////////////////////////////////////////////////////////

#define UsbSetupBuf ((USB_SETUP_REQ *)Ep0Buffer)

//////////////////////////////////////////////////////////////////////

uint8_t SetupReq;
uint8_t SetupLen;
uint8_t UsbConfig;

uint8_t const *pDescr;

volatile __idata uint8_t usb_idle = 3;
volatile __idata uint8_t usb_active = 0;

//////////////////////////////////////////////////////////////////////

// uint8_t media_key_report[3] = {
//     0x2,    //  8 bits: report ID 2
//     0x0,    // 16 bits: media key (0..7)
//     0x0     // 16 bits: media key (8..15)
// };

// uint8_t keyboard_report[8] = {
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
__code const uint8_t composite_rep_desc[] = {

    // KEYBOARD DEVICE

    0x05, 0x01,          // Usage Page: Generic Desktop Controls
    0x09, 0x06,          // Usage: Keyboard
    0xA1, 0x01,          // Collection: Application
    0x85, 0x01,          // REPORT_ID (1)
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
    0xC0,                // End collection

    // CONSUMER CONTROL DEVICE

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

//////////////////////////////////////////////////////////////////////

__code const uint8_t device_desc[] = {

    sizeof(device_desc),     // bLength
    USB_DESCR_TYP_DEVICE,    // bDescriptorType
    0x10,                    // bcdUSB (1)
    0x01,                    // bcdUSB (2)
    0x00,                    // bDeviceClass
    0x00,                    // bDeviceSubClass
    0x00,                    // bDeviceProtocol
    DEFAULT_ENDP0_SIZE,      // bMaxPacketSize0
    VENDOR_ID,               // idVendor (1,2)
    PRODUCT_ID,              // idProduct (1,2)
    0x00,                    // bcdDevice(1)
    0x01,                    // bcdDevice(2)
    0x01,                    // iManufacturer
    0x02,                    // iProduct
    0x03,                    // iSerialNumber
    0x01                     // bNumConfigurations
};

//////////////////////////////////////////////////////////////////////

__code const uint8_t config_desc[] = {

    // Config
    0x09,                          // bLength
    USB_DESCR_TYP_CONFIG,          // bDescriptorType
    sizeof(config_desc) & 0xff,    // wTotalLength (1)
    sizeof(config_desc) >> 8,      // wTotalLength (2)
    0x01,                          // bNumInterface
    0x01,                          // bConfigurationValue
    0x00,                          // iConfiguration
    0x80,                          // bmAttributes: Bus Power/No Remote Wakeup
    0x32,                          // bMaxPower

    // Interface
    0x09,                    // bLength
    USB_DESCR_TYP_INTERF,    // bDescriptorType
    0x00,                    // bInterfaceNumber
    0x00,                    // bAlternateSetting
    0x01,                    // bNumEndpoints
    USB_DEV_CLASS_HID,       // bInterfaceClass
    0x01,                    // bInterfaceSubClass
    0x01,                    // bInterfaceProtocol: Keyboard
    0x02,                    // iInterface

    // HID
    0x09,                                 // bLength
    USB_DESCR_TYP_HID,                    // bDescriptorType: HID
    0x11,                                 // bcdHID(1)
    0x01,                                 // bcdHID(2)
    0x00,                                 // bCountryCode
    0x01,                                 // bNumDescriptors
    0x22,                                 // bDescriptorType: Report
    sizeof(composite_rep_desc) & 0xff,    // wDescriptorLength (1)
    sizeof(composite_rep_desc) >> 8,      // wDescriptorLength (2)

    // Endpoint
    0x07,                         // bLength
    USB_DESCR_TYP_ENDP,           // bDescriptorType: ENDPOINT
    0x82,                         // bEndpointAddress: IN/Endpoint2
    0x03,                         // bmAttributes: Interrupt
    USB_PACKET_SIZE & 0xff,    // wMaxPacketSize (1)
    USB_PACKET_SIZE >> 8,      // wMaxPacketSize (2)
    0x02                         // bInterval
};
// clang-format on

//////////////////////////////////////////////////////////////////////

// String Descriptor (Language)
__code const unsigned char language_desc[] = { sizeof(language_desc), USB_DESCR_TYP_STRING, LANGUAGE_DESCRIPTION };

// String Descriptor (Manufacturer)
__code const unsigned char manufacturer_desc[] = { sizeof(manufacturer_desc), USB_DESCR_TYP_STRING, MANUFACTURER_DESCRIPTION };

//////////////////////////////////////////////////////////////////////

void usb_isr(void) __interrupt(INT_NO_USB)
{
    uint8_t len;

    if(UIF_TRANSFER) {

        switch(USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) {

            // [??] input (from us to host) for interface 1,2 arrived, we can send
            // more

        case UIS_TOKEN_IN | 1:
            UEP1_T_LEN = 0;
            UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            usb_idle |= 1;
            break;

        case UIS_TOKEN_IN | 2:
            UEP2_T_LEN = 0;
            UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            usb_idle |= 2;
            break;

            // setup request on interface 0

        case UIS_TOKEN_SETUP | 0:

            len = 0xff;

            // should be this much, anything else seems to be an error
            if(USB_RX_LEN == (sizeof(USB_SETUP_REQ))) {

                usb_active = 1;

                // how much it wants, capped at 127
                SetupLen = UsbSetupBuf->wLengthL;
                if(UsbSetupBuf->wLengthH || SetupLen > 0x7f) {
                    SetupLen = 0x7f;
                }

                len = 0;
                SetupReq = UsbSetupBuf->bRequest;
                uint8_t request_type = UsbSetupBuf->bRequestType;

                // all request types except 'standard' are ignored
                if((request_type & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {

                    switch(SetupReq) {

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

                } else {

                    // standard request handler
                    switch(SetupReq) {

                        // device request

                    case USB_GET_DESCRIPTOR:

                        switch(UsbSetupBuf->wValueH) {

                        case USB_DESCR_TYP_DEVICE:
                            pDescr = device_desc;
                            len = sizeof(device_desc);
                            break;

                        case USB_DESCR_TYP_CONFIG:
                            pDescr = config_desc;
                            len = sizeof(config_desc);
                            break;

                        case USB_DESCR_TYP_STRING: {
                            switch(UsbSetupBuf->wValueL) {
                            case 0:
                                pDescr = language_desc;
                                len = sizeof(language_desc);
                                break;

                            case 1:
                                pDescr = manufacturer_desc;
                                len = sizeof(manufacturer_desc);
                                break;

                            case 2:
                                pDescr = product_string;
                                len = PRODUCT_NAME_STRING_LEN;
                                break;
                            case 3:
                                pDescr = serial_number_string;
                                len = SERIAL_STRING_LEN;
                                break;
                            default:
                                len = 0xff;
                                break;
                            }
                        } break;

                        case USB_DESCR_TYP_REPORT:
                            switch(UsbSetupBuf->wIndexL) {
                            case 0:
                                pDescr = composite_rep_desc;
                                len = sizeof(composite_rep_desc);
                                break;
                            default:
                                len = 0xff;
                            }
                            break;

                        default:
                            len = 0xff;
                            break;
                        }

                        // continuation?
                        if(SetupLen > len) {
                            SetupLen = len;
                        }

                        // send at most ? bytes per request (coalesced on host)
                        len = MIN(SetupLen, DEFAULT_ENDP0_SIZE);
                        memcpy(Ep0Buffer, pDescr, len);
                        SetupLen -= len;
                        pDescr += len;
                        break;

                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;
                        break;

                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        SetupLen = MIN(SetupLen, 1);
                        break;

                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
                        break;

                    case USB_GET_INTERFACE:
                        break;

                    case USB_CLEAR_FEATURE:

                        if((request_type & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) {

                            switch(UsbSetupBuf->wIndexL) {

                            case USB_ENDP_DIR_MASK | 1:
                                UEP1_CTRL = (UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                                break;

                            case 1:
                                UEP1_CTRL = (UEP1_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                                break;

                            default:
                                len = 0xff;
                                break;
                            }

                        } else {

                            len = 0xff;
                        }
                        break;

                    case USB_SET_FEATURE:

                        switch(request_type & USB_REQ_RECIP_MASK) {

                        case USB_REQ_RECIP_DEVICE: {
                            if(*(uint16_t *)&UsbSetupBuf->wValueL == 0x0001) {

                                // asking to set remote wakeup!?
                                if(config_desc[7] & 0x20) {
                                    // len = 0;
                                } else {
                                    len = 0xff;
                                }
                            } else {
                                len = 0xff;
                            }
                        } break;

                        case USB_REQ_RECIP_ENDP: {
                            if(*(uint16_t *)&UsbSetupBuf->wValueL == 0x0000) {
                                switch(*(uint16_t *)&UsbSetupBuf->wIndexL) {
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;
                                    break;
                                default:
                                    len = 0xff;
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
                        len = MIN(SetupLen, 2);
                        break;

                    default:
                        len = 0xff;
                        break;
                    }
                }
            }

            // if len == 0xff, some error has occurred
            // set the data toggles to 1
            // and indicate stall
            if(len == 0xff) {
                SetupReq = 0xff;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;

            }
            // if len <= 8, some valid data is sitting in EP0Buffer
            // so send it (also set the toggles!?)
            // although it could be zero bytes, in which case we just ack it
            else if(len <= 8) {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
            }
            break;

            // endpoint0 IN

        case UIS_TOKEN_IN | 0:

            switch(SetupReq) {

            case USB_GET_DESCRIPTOR:
                len = MIN(SetupLen, 8);
                memcpy(Ep0Buffer, pDescr, len);
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;
                break;

            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;

            default:
                UEP0_T_LEN = 0;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;

            // endpoint 0 OUT

        case UIS_TOKEN_OUT | 0:
            UEP0_T_LEN = 0;
            UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_ACK;
            break;

        default:
            break;
        }
        UIF_TRANSFER = 0;
    }
    // USB bus hangs/Wake up
    if(UIF_SUSPEND) {
        UIF_SUSPEND = 0;
        // Hang up
        if(USB_MIS_ST & bUMS_SUSPEND) {
            putstr("suspend\n");
            while(XBUS_AUX & bUART0_TX) {    // Wait for msg to send
            }
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;    // USB or RXD0/1 can be awakened when there is a signal
            PCON |= PD;                                               // Sleep
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
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
    print_uint16("KEY", key);
    if(IS_MEDIA_KEY(key)) {
        key &= 0x7fff;
        Ep2Buffer[0] = 0x02;    // REPORT ID
        Ep2Buffer[1] = key & 0xff;
        Ep2Buffer[2] = key >> 8;
        UEP2_T_LEN = 3;
    } else {
        Ep2Buffer[0] = 0x01;    // REPORT ID
        Ep2Buffer[1] = 0x00;    // modifier
        Ep2Buffer[2] = 0x00;
        Ep2Buffer[3] = key;
        Ep2Buffer[4] = 0x00;
        Ep2Buffer[5] = 0x00;
        Ep2Buffer[6] = 0x00;
        Ep2Buffer[7] = 0x00;
        Ep2Buffer[8] = 0x00;
        UEP2_T_LEN = 9;
    }
    usb_idle &= ~2;
    UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
}

//////////////////////////////////////////////////////////////////////

void usb_device_config()
{
    USB_CTRL = 0x00;                                          // Clear USB control register
    USB_CTRL &= ~bUC_HOST_MODE;                               // This bit selects the device mode
    USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;    // The USB device and internal pull-up are enabled, and automatically return
                                                              // to NAK before the interrupt flag is cleared during the interrupt.
    USB_DEV_AD = 0x00;                                        // Device address initialization

    // USB_CTRL |= bUC_LOW_SPEED;
    // UDEV_CTRL |= bUD_LOW_SPEED;  // low speed 1.5Mbit mode
    USB_CTRL &= ~bUC_LOW_SPEED;
    UDEV_CTRL &= ~bUD_LOW_SPEED;    // full speed 12Mbit mode
    UDEV_CTRL |= bUD_PD_DIS;        // Disable DP/DM pull-down resistor
    UDEV_CTRL |= bUD_PORT_EN;       // Enable physical port
}

//////////////////////////////////////////////////////////////////////

void usb_device_int_config()
{
    USB_INT_EN |= bUIE_SUSPEND;     // Make the device hang up and interrupt
    USB_INT_EN |= bUIE_TRANSFER;    // Make USB transmission complete interruption
    USB_INT_EN |= bUIE_BUS_RST;     // Make the device mode USB bus reset and interrupt
    USB_INT_FG |= 0x1F;             // Clear interrupt
    IE_USB = 1;                     // Enable USB interrupt
    EA = 1;                         // Enable interrupts
}

//////////////////////////////////////////////////////////////////////

void usb_device_endpoint_config()
{
    UEP1_DMA = (uint16)Ep1Buffer;    // Point point 1 Send data transmission address
    UEP2_DMA = (uint16)Ep2Buffer;    // Writer 2 in data transmission address (endpoint_2_out_buffer must immediately follow
                                     // endpoint_2_in_buffer)
    UEP2_3_MOD = 0xCC;               // Endpoint 2/3 Single Single Single Receiving Fail

    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK |
                UEP_R_RES_ACK;    // Writing 2 automatically flip the synchronous flag position, IN transaction returns NAK, out of ACK

    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;    // Point 1 automatically flip the synchronization flag bit, IN transaction returns NAK
    UEP0_DMA = (uint16)Ep0Buffer;                 // Point 0 data transmission address
    UEP4_1_MOD = 0X40;    // Point point 1 upload the buffer area; endpoint 0 single 64 bytes receiving and receiving buffer
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;    // Flip manually, out of OUT transaction back to ACK, IN transaction returns Nak
}

//////////////////////////////////////////////////////////////////////

void usb_init()
{
    IE_USB = 0;
    USB_CTRL = 0x00;

    UEP0_DMA = (uint16_t)Ep0Buffer;
    UEP1_DMA = (uint16_t)Ep1Buffer;
    UEP2_DMA = (uint16_t)Ep2Buffer;

    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    UEP1_CTRL = bUEP_T_TOG | UEP_T_RES_NAK;
    UEP2_CTRL = bUEP_T_TOG | UEP_T_RES_NAK;

    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;
    UEP2_T_LEN = 0;

    UEP4_1_MOD = ~(bUEP4_RX_EN | bUEP4_TX_EN | bUEP1_RX_EN | bUEP1_BUF_MOD) | bUEP1_TX_EN;

    UEP2_3_MOD = ~(bUEP3_RX_EN | bUEP3_TX_EN | bUEP3_BUF_MOD | bUEP2_RX_EN | bUEP2_BUF_MOD) | bUEP2_TX_EN;

    USB_DEV_AD = 0x00;

    UDEV_CTRL = bUD_PD_DIS | UDEV_LOW_SPEED;

    USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN | UCTL_LOW_SPEED;

    UDEV_CTRL |= bUD_PORT_EN;

    USB_INT_FG = 0xff;

    USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;

    IE_USB = 1;
}
