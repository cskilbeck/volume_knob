#include <stdint.h>
#include <string.h>
#include "ch554.h"
#include "ch554_usb.h"
#include "debug.h"
#include "usb.h"

// //////////////////////////////////////////////////////////////////////

volatile __idata uint8 ep2_recv_len = 0;
volatile __idata uint8 ep2_busy = 0;
volatile __idata uint8 usb_config = 0;

uint8 const *descriptor = 0;
uint8 setup_request;
uint16 setup_len;

#define usb_setup_buffer ((PUSB_SETUP_REQ)Ep0Buffer)

// Device descriptor
__code uint8 const device_descriptor[] = {
    0x12,                  // length
    0x01,                  // USB_DESC_TYPE_DEVICE
    0x10,                  // USB v2.01 LOW BCD
    0x01,                  // USB v2.01 HIGH BCD
    0x00,                  // device class
    0x00,                  // device subclass
    0x00,                  // device protocol
    DEFAULT_ENDP0_SIZE,    // Define it in interface level
    USB_VID & 0xff,        // USB_VID (low)
    USB_VID >> 8,          // USB_VID (high)
    USB_PID & 0xff,        // USB_PID (low)
    USB_PID >> 8,          // USB_PID (high)
    0x00,                  // device rel LOW BCD
    0x01,                  // device rel HIGH BCD
    0x01,                  // manufacturer string id
    0x02,                  // product string id
    0x03,                  // serial number string id
    0x01                   // num configurations
};

__code uint8 const config_descriptor[] = {

    // Configuration descriptor (two interfaces)
    0x09, 0x02, sizeof(config_descriptor) & 0xff, sizeof(config_descriptor) >> 8, 0x02, 0x01, 0x00, 0x80, 0x32,

    // Interface 0 (audio interface) - USB Audio Class descriptor, no endpoint
    0x09, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,

    // Interface 0 function descriptor
    0x09, 0x24, 0x01, 0x00, 0x01, 0x09, 0x00, 0x01, 0x01,

    // Interface 1 - MIDI streaming interface descriptor
    0x09, 0x04, 0x01, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00,

    // Interface 1 function descriptor
    0x07, 0x24, 0x01, 0x00, 0x01, 0x25, 0x00,

    // IN-JACK
    0x06, 0x24, 0x02, 0x01, 0x01, 0x00,    //
    0x06, 0x24, 0x02, 0x02, 0x02, 0x00,    //

    // OUT-JACK
    0x09, 0x24, 0x03, 0x01, 0x03, 0x01, 0x02, 0x01, 0x00,    //
    0x09, 0x24, 0x03, 0x02, 0x04, 0x01, 0x01, 0x01, 0x00,    //

    // 2 endpoints
    0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,    // EMB MIDI JACK = 1, OUT
    0x05, 0x25, 0x01, 0x01, 0x01,                // Associated JACKID=1, OUT

    0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00,    // EMB MIDI JACK = 1, OUT
    0x05, 0x25, 0x01, 0x01, 0x03                 // Associated JACKID=3, IN

};

// language string
unsigned char __code const language_string[] = { 0x04, 0x03, 0x09, 0x04 };

// serial number string
unsigned char __code const serial_number_string[] = { 0x16, 0x03, '2',  0x00, '0',  0x00, '2',  0x00, '4',  0x00, '/',
                                                      0x00, '0',  0x00, '2',  0x00, '/',  0x00, '1',  0x00, '8',  0x00 };

// product string
unsigned char __code const product_string[] = {
    0x1E, 0x03, 'T',  0x00, 'i',  0x00, 'n',  0x00, 'y',  0x00, ' ',  0x00, 'M',  0x00, 'I',
    0x00, 'D',  0x00, 'I',  0x00, ' ',  0x00, 'K',  0x00, 'n',  0x00, 'o',  0x00, 'b',  0x00,
};

// manufacturer string
unsigned char __code const manufacturer_string[] = { 0x26, 0x03, 'T', 0,   'i', 0,   'n', 0,   'y', 0,   ' ', 0,   'L', 0,   'i', 0,   't', 0,   't',
                                                     0,    'l',  0,   'e', 0,   ' ', 0,   'T', 0,   'h', 0,   'i', 0,   'n', 0,   'g', 0,   's', 0 };

//////////////////////////////////////////////////////////////////////

void usb_device_config()
{
    USB_CTRL = 0x00;                                          // Clear USB control register
    USB_CTRL &= ~bUC_HOST_MODE;                               // This bit selects the device mode
    USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;    // The USB device and internal pull-up are enabled, and automatically return to NAK before the
                                                              // interrupt flag is cleared during the interrupt.
    USB_DEV_AD = 0x00;                                        // Device address initialization
    //	 USB_CTRL |= bUC_LOW_SPEED;
    //	 UDEV_CTRL |= bUD_LOW_SPEED;												//Select low speed 1.5M mode
    USB_CTRL &= ~bUC_LOW_SPEED;
    UDEV_CTRL &= ~bUD_LOW_SPEED;    // Select full speed 12M mode, the default mode
    UDEV_CTRL = bUD_PD_DIS;         // Disable DP/DM pull-down resistor
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
    UEP2_DMA = (uint16)Ep2Buffer;    // Writer 2 in data transmission address
    UEP2_3_MOD = 0xCC;               // Endpoint 2/3 Single Single Single Receiving Fail
    UEP2_CTRL =
        bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;    // Writing 2 automatically flip the synchronous flag position, IN transaction returns NAK, out of ACK

    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;    // Point 1 automatically flip the synchronization flag bit, IN transaction returns NAK
    UEP0_DMA = (uint16)Ep0Buffer;                 // Point 0 data transmission address
    UEP4_1_MOD = 0X40;                            // Point point 1 upload the buffer area; endpoint 0 single 64 bytes receiving and receiving buffer
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;    // Flip manually, out of OUT transaction back to ACK, IN transaction returns Nak
}

//////////////////////////////////////////////////////////////////////

void usb_irq_handler(void) __interrupt(INT_NO_USB)    // USB interrupt service program, use the register group 1
{
    uint16 len;

    if(UIF_TRANSFER) {

        switch(USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) {

        case UIS_TOKEN_IN | 1:
            UEP1_T_LEN = 0;
            UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            break;

        case UIS_TOKEN_IN | 2:
            UEP2_T_LEN = 0;
            UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            ep2_busy = 0;
            break;

        case UIS_TOKEN_OUT | 2:
            if(U_TOG_OK) {
                ep2_recv_len = USB_RX_LEN;
                UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_NAK;
            }
            break;

        case UIS_TOKEN_SETUP | 0:
            len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ))) {
                setup_len = ((uint16)usb_setup_buffer->wLengthH << 8) | (usb_setup_buffer->wLengthL);
                len = 0;    // The default is success and upload 0 length

                setup_request = usb_setup_buffer->bRequest;

                if((usb_setup_buffer->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {    // Non-standard request
                    len = 0xFF;                                                                      // unsupported
                } else {                                                                             // Standard request
                    switch(setup_request)                                                            // Request code
                    {
                    case USB_GET_DESCRIPTOR:

                        switch(usb_setup_buffer->wValueH) {

                        case USB_DESCR_TYP_DEVICE:    // Device descriptor
                            descriptor = device_descriptor;
                            len = sizeof(device_descriptor);
                            break;

                        case USB_DESCR_TYP_CONFIG:    // Configuration descriptor
                            descriptor = config_descriptor;
                            len = sizeof(config_descriptor);
                            break;

                        case USB_DESCR_TYP_STRING:    // String descriptor

                            switch(usb_setup_buffer->wValueL) {
                            case 0:
                                descriptor = language_string;
                                len = sizeof(language_string);
                                break;
                            case 1:
                                descriptor = manufacturer_string;
                                len = sizeof(manufacturer_string);
                                break;
                            case 2:
                                descriptor = product_string;
                                len = sizeof(product_string);
                                break;
                            default:
                                descriptor = serial_number_string;
                                len = sizeof(serial_number_string);
                                break;
                            }
                            break;

                        default:
                            len = 0xff;    // unsupported
                            break;
                        }
                        if(setup_len > len) {
                            setup_len = len;    // Limit total length
                        }
                        len = setup_len >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : setup_len;    // This transmission length
                        memcpy(Ep0Buffer, descriptor, len);                                        // Load upload data
                        setup_len -= len;
                        descriptor += len;
                        break;

                    case USB_SET_ADDRESS:
                        setup_len = usb_setup_buffer->wValueL;    // Temporary USB device address
                        break;

                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = usb_config;
                        if(setup_len >= 1) {
                            len = 1;
                        }
                        break;

                    case USB_SET_CONFIGURATION:
                        usb_config = usb_setup_buffer->wValueL;
                        hexdump("CONFIG", &usb_config, 1);
                        break;

                    case USB_GET_INTERFACE:
                        break;

                    case USB_CLEAR_FEATURE:                                                    // Clear Feature
                        if((usb_setup_buffer->bRequestType & 0x1F) == USB_REQ_RECIP_DEVICE)    // Device
                        {
                            if((((uint16)usb_setup_buffer->wValueH << 8) | usb_setup_buffer->wValueL) == 0x01) {
                                if(config_descriptor[7] & 0x20) {
                                    // wake
                                } else {
                                    len = 0xFF;    // operation failed
                                }
                            } else {
                                len = 0xFF;    // operation failed
                            }
                        } else if((usb_setup_buffer->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)    // Endpoint
                        {
                            switch(usb_setup_buffer->wIndexL) {
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
                        } else {
                            len = 0xFF;    // unsupported
                        }
                        break;
                    case USB_SET_FEATURE:                                                      // Set Feature
                        if((usb_setup_buffer->bRequestType & 0x1F) == USB_REQ_RECIP_DEVICE)    // Device
                        {
                            if((((uint16)usb_setup_buffer->wValueH << 8) | usb_setup_buffer->wValueL) == 0x01) {
                                if(config_descriptor[7] & 0x20) {
                                    // Dormant
                                    while(XBUS_AUX & bUART0_TX) {
                                        ;    // Waiting for sending complete
                                    }
                                    SAFE_MOD = 0x55;
                                    SAFE_MOD = 0xAA;
                                    WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;    // USB or RXD0/1 When there is a signal, it can be awakened
                                    PCON |= PD;                                               // Sleep
                                    SAFE_MOD = 0x55;
                                    SAFE_MOD = 0xAA;
                                    WAKE_CTRL = 0x00;
                                } else {
                                    len = 0xFF;    // operation failed
                                }
                            } else {
                                len = 0xFF;    // operation failed
                            }
                        } else if((usb_setup_buffer->bRequestType & 0x1F) == USB_REQ_RECIP_ENDP)    // Set the endpoint
                        {
                            if((((uint16)usb_setup_buffer->wValueH << 8) | usb_setup_buffer->wValueL) == 0x00) {
                                switch(((uint16)usb_setup_buffer->wIndexH << 8) | usb_setup_buffer->wIndexL) {
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
                                len = 0xFF;    // operation failed
                            }
                        } else {
                            len = 0xFF;    // operation failed
                        }
                        break;
                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;
                        if(setup_len >= 2) {
                            len = 2;
                        } else {
                            len = setup_len;
                        }
                        break;
                    default:
                        len = 0xff;    // operation failed
                        break;
                    }
                }
            } else {
                len = 0xff;
            }
            if(len == 0xff) {
                setup_request = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;    // STALL
            } else if(len <= DEFAULT_ENDP0_SIZE) {                                          // Upload data or status stage Return 0 length package
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;    // The default data packet is data1, and return to ACK
            } else {
                UEP0_T_LEN = 0;    // Although it has not yet reached the state stage, upload 0 length packets in advance to prevent the host from entering the
                                   // state stage in advance
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;    // The default data packet is data1, and return to ACK
            }
            break;
        case UIS_TOKEN_IN | 0:    // endpoint0 IN
            switch(setup_request) {
            case USB_GET_DESCRIPTOR:
                len = setup_len >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : setup_len;    // This transmission length
                memcpy(Ep0Buffer, descriptor, len);                                        // Load upload data
                setup_len -= len;
                descriptor += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;    // Synchronous flag flip
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | setup_len;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0;    // Complete interruption or forced upload 0-length packets to end control transmission during the state stage
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0:
            UEP0_T_LEN = 0;
            UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK;    // State stage, in response to NAK
            break;

        default:
            break;
        }
        UIF_TRANSFER = 0;    // clear interrupt
    }

    if(UIF_BUS_RST)    // bus reset
    {
        putstr("reset\n");    // Sleep state
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        usb_config = 0;    // Clear configuration value
        ep2_busy = 0;
    }
    if(UIF_SUSPEND)    // USB bus hangs/Wake up
    {
        UIF_SUSPEND = 0;
        if(USB_MIS_ST & bUMS_SUSPEND)    // Hang up
        {
            putstr("suspend\n");    // Sleep state
            while(XBUS_AUX & bUART0_TX) {
                ;    // Waiting for sending complete
            }
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;    // USB or RXD0/1 can be awakened when there is a signal
            PCON |= PD;                                               // Sleep
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = 0x00;
            putstr("awoke\n");
        }
    } else {                  // Unexpected interrupt, should not happen
        USB_INT_FG = 0xFF;    // Clear interruption
    }
}
