#include <stdint.h>
#include <string.h>

#include <ch554.h>
#include <ch554_usb.h>

#include "hid.h"

#define VENDOR_ID 0xD0, 0x16     // VID = 16D0
#define PRODUCT_ID 0x4B, 0x11    // PID = 114B

#define MANUFACTURER_DESCRIPTION 'C', 0x00, 'H', 0x00, ' ', 0x00, 'S', 0x00, 'k', 0x00, 'i', 0x00, 'l', 0x00, 'b', 0x00, 'e', 0x00, 'c', 0x00, 'k', 0x00

#define PRODUCT_DESCRIPTION 'V', 0x00, 'o', 0x00, 'l', 0x00, 'u', 0x00, 'm', 0x00, 'e', 0x00, 'K', 0x00, 'n', 0x00, 'o', 0x00, 'b', 0x00

// Memory map:
// EP0 Buf     00 - 07
// EP1 Buf     10 - 4f

#define FIXED_ADDRESS_EP0_BUFFER 0x0000
#define FIXED_ADDRESS_EP1_BUFFER 0x0010

#define UsbSetupBuf ((USB_SETUP_REQ *)Ep0Buffer)

__xdata __at(FIXED_ADDRESS_EP0_BUFFER) uint8_t Ep0Buffer[DEFAULT_ENDP0_SIZE];    // Endpoint0 OUT & IN
__xdata __at(FIXED_ADDRESS_EP1_BUFFER) uint8_t Ep1Buffer[MAX_PACKET_SIZE];       // Endpoint1 IN

uint8_t SetupReq, SetupLen, UsbConfig;
USB_SETUP_REQ SetupReqBuf;
__code uint8_t *pDescr;

volatile __idata uint8_t RepDescSent;
volatile __idata uint8_t usb_idle = 1;

uint8_t media_key_report[3] = {
    0x2,    // report ID 1
    0x0,    // media keys defined above
    0x0,    // pad set this to 0
};

// Device Descriptor
__code uint8_t DevDesc[18] = {
    0x12,                  // bLength
    0x01,                  // bDescriptorType: DEVICE
    0x10,                  // bcdUSB: USB1.1 (1)
    0x01,                  // bcdUSB: USB1.1 (2)
    0x00,                  // bDeviceClass
    0x00,                  // bDeviceSubClass
    0x00,                  // bDeviceProtocol
    DEFAULT_ENDP0_SIZE,    // bMaxPacketSize0
    VENDOR_ID,             // idVendor
    PRODUCT_ID,            // idProduct
    0x00,                  // bcdDevice(1)
    0x01,                  // bcdDevice(2)
    0x01,                  // iManufacturer
    0x02,                  // iProduct
    0x00,                  // iSerialNumber
    0x01                   // bNumConfigurations
};

// Configuration Descriptor
__code uint8_t CfgDesc[34] = {

    // Device
    0x09,                    // bLength
    USB_DESCR_TYP_CONFIG,    // bDescriptorType: CONFIGURATION
    0x22, 0x00,              // wTotalLength
    0x01,                    // bNumInterface
    0x01,                    // bConfigurationValue
    0x00,                    // iConfiguration
    0x80,                    // bmAttributes: Bus Power/No Remote Wakeup
    0x32,                    // bMaxPower

    // Interface
    0x09,                    // bLength
    USB_DESCR_TYP_INTERF,    // bDescriptorType: INTERFACE
    0x00,                    // bInterfaceNumber
    0x00,                    // bAlternateSetting
    0x01,                    // bNumEndpoints
    USB_DEV_CLASS_HID,       // bInterfaceClass: HID
    0x01,                    // bInterfaceSubClass
    0x01,                    // bInterfaceProtocol: Keyboard
    0x00,                    // iInterface

    // HID
    0x09,                 // bLength
    USB_DESCR_TYP_HID,    // bDescriptorType: HID
    0x11, 0x01,           // bcdHID: 1.10
    0x00,                 // bCountryCode
    0x01,                 // bNumDescriptors
    0x22,                 // bDescriptorType: Report
    28, 0,                // wDescriptorLength: 28

    // Endpoint
    0x07,                      // bLength
    USB_DESCR_TYP_ENDP,        // bDescriptorType: ENDPOINT
    0x81,                      // bEndpointAddress: IN/Endpoint1
    0x03,                      // bmAttributes: Interrupt
    MAX_PACKET_SIZE & 0xff,    // wMaxPacketSize (1)
    MAX_PACKET_SIZE >> 8,      // wMaxPacketSize (2)
    0x02,                      // bInterval
};

__code uint8_t MediaRepDesc[28] = {
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
    0x81, 0x00,                      //   INPUT (Data,Ary,Abs)
    0xc0                             // END_COLLECTION
};

// String Descriptor (Language)
__code unsigned char LangDesc[] = { 0x04, 0x03, 0x09, 0x04 };

// String Descriptor (Manufacturer)
__code unsigned char ManufDesc[] = { sizeof(ManufDesc), 0x03, MANUFACTURER_DESCRIPTION };

// String Descritor (Product)
__code unsigned char ProdDesc[] = { sizeof(ProdDesc), 0x03, PRODUCT_DESCRIPTION };

void usb_isr(void) __interrupt(INT_NO_USB)
{
    uint8_t len;

    if(UIF_TRANSFER) {
        switch(USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) {
        case UIS_TOKEN_IN | 1:
            UEP1_T_LEN = 0;
            UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;
            usb_idle = 1;
            break;
        case UIS_TOKEN_SETUP | 0:
            len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ))) {
                SetupLen = UsbSetupBuf->wLengthL;
                if(UsbSetupBuf->wLengthH || SetupLen > 0x7f) {
                    SetupLen = 0x7f;
                }
                len = 0;
                SetupReq = UsbSetupBuf->bRequest;
                if((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {
                    switch(SetupReq) {
                    case 0x01:    // GetReport
                        break;
                    case 0x02:    // GetIdle
                        break;
                    case 0x03:    // GetProtocol
                        break;
                    case 0x09:    // SetReport
                        break;
                    case 0x0a:    // SetIdle
                        break;
                    case 0x0b:    // SetProtocol
                        break;
                    default:
                        len = 0xff;
                        break;
                    }
                } else {
                    switch(SetupReq) {
                    case USB_GET_DESCRIPTOR:
                        switch(UsbSetupBuf->wValueH) {
                        case 1:
                            pDescr = DevDesc;
                            len = sizeof(DevDesc);
                            break;
                        case 2:
                            pDescr = CfgDesc;
                            len = sizeof(CfgDesc);
                            break;
                        case 3:
                            if(UsbSetupBuf->wValueL == 0) {
                                pDescr = LangDesc;
                                len = sizeof(LangDesc);
                            } else if(UsbSetupBuf->wValueL == 1) {
                                pDescr = ManufDesc;
                                len = sizeof(ManufDesc);
                            } else if(UsbSetupBuf->wValueL == 2) {
                                pDescr = ProdDesc;
                                len = sizeof(ProdDesc);
                            } else {
                                len = 0xff;
                            }
                            break;

                        case 0x22:
                            if(UsbSetupBuf->wIndexL == 0) {
                                pDescr = MediaRepDesc;
                                len = sizeof(MediaRepDesc);
                                RepDescSent = 1;
                            } else {
                                len = 0xff;
                            }
                            break;
                        default:
                            len = 0xff;
                            break;
                        }
                        if(SetupLen > len) {
                            SetupLen = len;
                        }
                        len = SetupLen >= 8 ? 8 : SetupLen;
                        memcpy(Ep0Buffer, pDescr, len);
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if(SetupLen >= 1) {
                            len = 1;
                        }
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
                        break;
                    case 0x0a:
                        break;
                    case USB_CLEAR_FEATURE:
                        if((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) {
                            switch(UsbSetupBuf->wIndexL) {
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                break;
                            case 0x01:
                                UEP1_CTRL = UEP1_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
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
                        if((UsbSetupBuf->bRequestType & 0x1f) == 0x00) {
                            if((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01) {
                                if(CfgDesc[7] & 0x20) {
                                } else {
                                    len = 0xff;
                                }
                            } else {
                                len = 0xff;
                            }
                        } else if((UsbSetupBuf->bRequestType & 0x1f) == 0x02) {
                            if((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x00) {
                                switch(((uint16_t)UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL) {
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
                        } else {
                            len = 0xff;
                        }
                        break;
                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;
                        if(SetupLen >= 2) {
                            len = 2;
                        } else {
                            len = SetupLen;
                        }
                        break;
                    default:
                        len = 0xff;
                        break;
                    }
                }
            } else {
                len = 0xff;
            }
            if(len == 0xff) {
                SetupReq = 0xff;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
            } else if(len <= 8) {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
            } else {
                UEP0_T_LEN = 0;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
            }
            break;
        case UIS_TOKEN_IN | 0:    // endpoint0 IN
            switch(SetupReq) {
            case USB_GET_DESCRIPTOR:
                len = SetupLen >= 8 ? 8 : SetupLen;
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
        case UIS_TOKEN_OUT | 0:    // endpoint0 OUT
            UEP0_T_LEN = 0;
            UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_ACK;
            break;
        default:
            break;
        }
        UIF_TRANSFER = 0;
    }
    if(UIF_BUS_RST) {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        UIF_BUS_RST = 0;
    }
    if(UIF_SUSPEND) {
        UIF_SUSPEND = 0;
    } else {
        USB_INT_FG = 0xff;
    }
}

void usb_set_keystate(uint8_t key)
{
    Ep1Buffer[1] = key;
    usb_idle = 0;
    UEP1_T_LEN = sizeof(media_key_report);
    UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
}

void usb_init()
{
    uint8_t *p = Ep1Buffer;

    *p++ = (uint8_t)0x2;    // report ID 1
    *p++ = (uint8_t)0x0;    // media key
    *p++ = (uint8_t)0x0;    // pad, set this to 0

    IE_USB = 0;
    USB_CTRL = 0x00;
    UEP0_DMA = (uint16_t)Ep0Buffer;
    UEP1_DMA = (uint16_t)Ep1Buffer;
    UEP4_1_MOD = ~(bUEP4_RX_EN | bUEP4_TX_EN | bUEP1_RX_EN | bUEP1_BUF_MOD) | bUEP4_TX_EN;
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    UEP1_CTRL = bUEP_T_TOG | UEP_T_RES_NAK;
    UEP1_T_LEN = 0;

    USB_DEV_AD = 0x00;
    UDEV_CTRL = bUD_PD_DIS;
    USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;

    UDEV_CTRL |= bUD_PORT_EN;
    USB_INT_FG = 0xff;
    USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
    IE_USB = 1;

    RepDescSent = 0;
    EA = 1;
    while(RepDescSent == 0) {
    };
}
