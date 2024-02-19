/********************************** (C) COPYRIGHT *******************************
 * File Name		: main.c
 * Author			: Zhiyuan Wan
 * License			: MIT
 * Version			: V1.0
 * Date				: 2018/03/27
 * Description		: USB-MIDI
 *******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ch554.h>
#include <ch554_usb.h>
#include <debug.h>

enum midi_code_index
{
    mci_misc_function = 0x0,       // 1, 2 or 3    Miscellaneous function codes
    mci_cable_event = 0x1,         // 1, 2 or 3    Reserved for future expansion
    mci_system_common_2 = 0x2,     // 2            Two-byte System Common messages like MTC, SongSelect, etc.
    mci_system_common_3 = 0x3,     // 3            Three-byte System Common messages like SPP, etc.
    mci_sysex_start = 0x4,         // 3            SysEx starts or continues
    mci_sysex_end_1 = 0x5,         // 1            Single-byte System Common Message or SysEx ends with following single byte.
    mci_sysex_end_2 = 0x6,         // 2            SysEx ends with following two bytes.
    mci_sysex_end_3 = 0x7,         // 3            SysEx ends with following three bytes.
    mci_note_off = 0x8,            // 3            Note-off
    mci_note_on = 0x9,             // 3            Note-on
    mci_poly_keypress = 0xA,       // 3            Poly-KeyPress
    mci_control_change = 0xB,      // 3            Control Change
    mci_program_change = 0xC,      // 2            Program Change
    mci_channel_pressure = 0xD,    // 2            Channel Pressure
    mci_pitch_bend = 0xE,          // 3            PitchBend Change
    mci_single_byte = 0xF,         // 1            Single Byte
};

__xdata __at(0x0000) uint8_t Ep0Buffer[DEFAULT_ENDP0_SIZE];     // endpoint0 OUT & IN buffer，Must be an even address
__xdata __at(0x0040) uint8_t Ep1Buffer[DEFAULT_ENDP1_SIZE];     // endpoint1 upload buffer
__xdata __at(0x0080) uint8_t Ep2Buffer[2 * MAX_PACKET_SIZE];    // endpoint2 IN & OUT buffer, Must be an even address

// VID = 16D0, PID = 1317

#define USB_VID 0x16D0
#define USB_PID 0x1317

uint16_t SetupLen;
uint8_t SetupReq;
uint8_t UsbConfig;
const uint8_t *pDescr;    // USB descriptor to send

#define UsbSetupBuf ((PUSB_SETUP_REQ)Ep0Buffer)

#define USB_DESC_TYPE_DEVICE 0x01

// Device descriptor
__code uint8_t DevDesc[] = {
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

__code uint8_t CfgDesc[] = {

    // Configuration descriptor (two interfaces)
    0x09, 0x02, sizeof(CfgDesc) & 0xff, sizeof(CfgDesc) >> 8, 0x02, 0x01, 0x00, 0x80, 0x32,

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
unsigned char __code language_string[] = { 0x04, 0x03, 0x09, 0x04 };

// serial number string
unsigned char __code serial_number_string[] = { 0x16, 0x03, '2',  0x00, '0',  0x00, '2',  0x00, '4',  0x00, '/',
                                                0x00, '0',  0x00, '2',  0x00, '/',  0x00, '1',  0x00, '8',  0x00 };

// product string
unsigned char __code product_string[] = {
    0x1E, 0x03, 'T',  0x00, 'i',  0x00, 'n',  0x00, 'y',  0x00, ' ',  0x00, 'M',  0x00, 'I',
    0x00, 'D',  0x00, 'I',  0x00, ' ',  0x00, 'K',  0x00, 'n',  0x00, 'o',  0x00, 'b',  0x00,
};

// manufacturer string
unsigned char __code manufacturer_string[] = { 0x26, 0x03, 'T', 0,   'i', 0,   'n', 0,   'y', 0,   ' ', 0,   'L', 0,   'i', 0,   't', 0,   't',
                                               0,    'l',  0,   'e', 0,   ' ', 0,   'T', 0,   'h', 0,   'i', 0,   'n', 0,   'g', 0,   's', 0 };

#define MIDI_REV_LEN 64                          // MIDI receive buffer size
__idata uint8_t midi_in_buffer[MIDI_REV_LEN];    // MIDI receive buffer

volatile __idata uint8_t ep2_recv_len = 0;    // # received by USB endpoint
volatile __idata uint8_t ep2_busy = 0;        // upload endpoint busy flag

/*******************************************************************************
 * Function Name  : USBDeviceCfg()
 * Description	: USB Device mode configuration
 * Input		  : None
 * Output		 : None
 * Return		 : None
 *******************************************************************************/

void USBDeviceCfg()
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
/*******************************************************************************
 * Function Name  : USBDeviceIntCfg()
 * Description	: USB Device mode interrupt initialization
 * Input		  : None
 * Output		 : None
 * Return		 : None
 *******************************************************************************/
void USBDeviceIntCfg()
{
    USB_INT_EN |= bUIE_SUSPEND;     // Make the device hang up and interrupt
    USB_INT_EN |= bUIE_TRANSFER;    // Make USB transmission complete interruption
    USB_INT_EN |= bUIE_BUS_RST;     // Make the device mode USB bus reset and interrupt
    USB_INT_FG |= 0x1F;             // Clear interrupt
    IE_USB = 1;                     // Enable USB interrupt
    EA = 1;                         // Enable interrupts
}
/*******************************************************************************
 * Function Name    : USBDeviceEndPointCfg()
 * Description	    : USB device mode endpoint configuration
 *                  : simulation compatible HID device,
 *                  : in addition to the control transmission of endpoint 0, also includes endpoint 2 batch up and down
 * Input		    : None
 * Output		    : None
 * Return		    : None
 *******************************************************************************/
void USBDeviceEndPointCfg()
{
    UEP1_DMA = (uint16_t)Ep1Buffer;    // Point point 1 Send data transmission address
    UEP2_DMA = (uint16_t)Ep2Buffer;    // Writer 2 in data transmission address
    UEP2_3_MOD = 0xCC;                 // Endpoint 2/3 Single Single Single Receiving Fail
    UEP2_CTRL =
        bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;    // Writing 2 automatically flip the synchronous flag position, IN transaction returns NAK, out of ACK

    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;    // Point 1 automatically flip the synchronization flag bit, IN transaction returns NAK
    UEP0_DMA = (uint16_t)Ep0Buffer;               // Point 0 data transmission address
    UEP4_1_MOD = 0X40;                            // Point point 1 upload the buffer area; endpoint 0 single 64 bytes receiving and receiving buffer
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;    // Flip manually, out of OUT transaction back to ACK, IN transaction returns Nak
}

void putnibble(uint8_t n)
{
    if(n > 9) {
        n += 'A' - 10;
    } else {
        n += '0';
    }
    putchar(n);
}

void putstr(char *p)
{
    while(*p != 0) {
        putchar(*p++);
    }
}

void puthex(uint8_t b)
{
    putnibble(b >> 4);
    putnibble(b & 0xf);
}

void hexdump(char *msg, uint8_t *p, uint8_t n)
{
    putstr(msg);
    putchar(':');
    while(n-- != 0) {
        puthex(*p++);
    }
    putchar('\n');
}

/*******************************************************************************
 * Function Name  : DeviceInterrupt()
 * Description	: CH55XUSB interrupt processing function
 *******************************************************************************/

void DeviceInterrupt(void) __interrupt(INT_NO_USB)    // USB interrupt service program, use the register group 1
{
    uint16_t len;

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
                SetupLen = ((uint16_t)UsbSetupBuf->wLengthH << 8) | (UsbSetupBuf->wLengthL);
                len = 0;    // The default is success and upload 0 length

                SetupReq = UsbSetupBuf->bRequest;

                if((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {    // Non-standard request
                    len = 0xFF;                                                                 // unsupported
                } else {                                                                        // Standard request
                    switch(SetupReq)                                                            // Request code
                    {
                    case USB_GET_DESCRIPTOR:

                        switch(UsbSetupBuf->wValueH) {

                        case USB_DESCR_TYP_DEVICE:    // Device descriptor
                            pDescr = DevDesc;
                            len = sizeof(DevDesc);
                            break;

                        case USB_DESCR_TYP_CONFIG:    // Configuration descriptor
                            pDescr = CfgDesc;
                            len = sizeof(CfgDesc);
                            break;

                        case USB_DESCR_TYP_STRING:    // String descriptor

                            switch(UsbSetupBuf->wValueL) {
                            case 0:
                                pDescr = language_string;
                                len = sizeof(language_string);
                                break;
                            case 1:
                                pDescr = manufacturer_string;
                                len = sizeof(manufacturer_string);
                                break;
                            case 2:
                                pDescr = product_string;
                                len = sizeof(product_string);
                                break;
                            default:
                                pDescr = serial_number_string;
                                len = sizeof(serial_number_string);
                                break;
                            }
                            break;

                        default:
                            len = 0xff;    // unsupported
                            break;
                        }
                        if(SetupLen > len) {
                            SetupLen = len;    // Limit total length
                        }
                        len = SetupLen >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : SetupLen;    // This transmission length
                        memcpy(Ep0Buffer, pDescr, len);                                          // Load upload data
                        SetupLen -= len;
                        pDescr += len;
                        break;

                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;    // Temporary USB device address
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

                    case USB_GET_INTERFACE:
                        break;

                    case USB_CLEAR_FEATURE:                                               // Clear Feature
                        if((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_DEVICE)    // Device
                        {
                            if((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01) {
                                if(CfgDesc[7] & 0x20) {
                                    // wake
                                } else {
                                    len = 0xFF;    // operation failed
                                }
                            } else {
                                len = 0xFF;    // operation failed
                            }
                        } else if((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)    // Endpoint
                        {
                            switch(UsbSetupBuf->wIndexL) {
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
                    case USB_SET_FEATURE:                                                 // Set Feature
                        if((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_DEVICE)    // Device
                        {
                            if((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01) {
                                if(CfgDesc[7] & 0x20) {
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
                        } else if((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_ENDP)    // Set the endpoint
                        {
                            if((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x00) {
                                switch(((uint16_t)UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL) {
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
                        if(SetupLen >= 2) {
                            len = 2;
                        } else {
                            len = SetupLen;
                        }
                        break;
                    default:
                        len = 0xff;    // operation failed
                        break;
                    }
                }
            } else {
                len = 0xff;    // Bar length error
            }
            if(len == 0xff) {
                SetupReq = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;    // STALL
            } else if(len <= DEFAULT_ENDP0_SIZE)                                            // Upload data or status stage Return 0 length package
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;    // The default data packet is data1, and return to ACK
            } else {
                UEP0_T_LEN = 0;    // Although it has not yet reached the state stage, upload 0 length packets in advance to prevent the host from entering the
                                   // state stage in advance
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;    // The default data packet is data1, and return to ACK
            }
            break;
        case UIS_TOKEN_IN | 0:    // endpoint0 IN
            switch(SetupReq) {
            case USB_GET_DESCRIPTOR:
                len = SetupLen >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : SetupLen;    // This transmission length
                memcpy(Ep0Buffer, pDescr, len);                                          // Load upload data
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;    // Synchronous flag flip
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0;    // Complete interruption or forced upload 0 -length packets to end control transmission during the state stage
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
        UIF_BUS_RST = 0;     // Clear interrupt
        ep2_recv_len = 0;    // The length received by USB endpoint
        UsbConfig = 0;       // Clear configuration value
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
        }
    } else {                  // Unexpected interrupt, should not happen
        USB_INT_FG = 0xFF;    // Clear interruption
    }
}

//////////////////////////////////////////////////////////////////////

bool in_sysex = false;
uint8_t sysex_buffer[16];
uint8_t sysex_length = 0;

typedef enum
{
    sysex_end = 0,
    sysex_more = 1
} sysex_continue;

uint8_t packet_offset = 0;

void add_sysex(uint8_t length, sysex_continue done)
{
    if(length <= (sizeof(sysex_buffer) - sysex_length)) {
        memcpy(sysex_buffer + sysex_length, midi_in_buffer + packet_offset + 1, length);
        sysex_length += length;
    }
    if(done == sysex_end) {
        if(sysex_length == 11 && sysex_buffer[0] == 0xF0 && sysex_buffer[1] == 0x7F && sysex_buffer[10] == 0xF7) {
            uint8_t top_bits = sysex_buffer[9];
            for(uint8_t i = 2; i < 9; ++i) {
                top_bits <<= 1;
                sysex_buffer[i] |= top_bits & 0x80;
            }
        }
        hexdump("all", sysex_buffer + 2, 7);
        sysex_length = 0;
    }
}

void process_midi_packet(uint8_t length)
{
    packet_offset = 0;
    while(packet_offset < length) {

        hexdump("id", midi_in_buffer + packet_offset, 1);

        switch(midi_in_buffer[packet_offset]) {

        case mci_sysex_start:
            add_sysex(3, sysex_more);
            break;

        case mci_sysex_end_1:
            add_sysex(1, sysex_end);
            break;

        case mci_sysex_end_2:
            add_sysex(2, sysex_end);
            break;

        case mci_sysex_end_3:
            add_sysex(3, sysex_end);
            break;

        default:
            sysex_length = 0;
            break;
        }
        packet_offset += 4;
    }
}

//////////////////////////////////////////////////////////////////////
// MAIN

//////////////////////////////////////////////////////////////////////

typedef int8_t int8;
typedef int16_t int16;
typedef uint8_t uint8;
typedef uint16_t uint16;

#define PORT1 0x90
#define PORT3 0xB0

#define UART_TX_PORT PORT3
#define UART_TX_PIN 0

#define UART_RX_PORT PORT3
#define UART_RX_PIN 1

#define ROTA_PORT PORT3
#define ROTA_PIN 3

#define ROTB_PORT PORT3
#define ROTB_PIN 4

#define BTN_PORT PORT3
#define BTN_PIN 2

#define LED_PORT PORT1
#define LED_PIN 6

SBIT(BTN_BIT, BTN_PORT, BTN_PIN);
SBIT(LED_BIT, LED_PORT, LED_PIN);
SBIT(ROTA_BIT, ROTA_PORT, ROTA_PIN);
SBIT(ROTB_BIT, ROTB_PORT, ROTB_PIN);

#define CLOCKWISE 2
#define ANTI_CLOCKWISE 0

uint8 vol_direction;
int8 turn_value;

// Define ROTARY_DIRECTION as CLOCKWISE for one kind of encoders, ANTI_CLOCKWISE for
// the other ones (some are reversed). This sets the default rotation (after first
// flash), reverse by triple-clicking the knob

#define ROTARY_DIRECTION (CLOCKWISE)
// #define ROTARY_DIRECTION (ANTI_CLOCKWISE)

//////////////////////////////////////////////////////////////////////
// rotary encoder reader

// odd # of bits set (1 or 3) means it's valid

// 0  00 00 0
// 1  00 01 1
// 2  00 10 1
// 3  00 11 0
// 4  01 00 1
// 5  01 01 0
// 6  01 10 0
// 7  01 11 1
// 8  10 00 1
// 9  10 01 0
// A  10 10 0
// B  10 11 1
// C  11 00 0
// D  11 01 1
// E  11 10 1
// F  11 11 0

uint8 const encoder_valid_bits[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

uint8 encoder_state = 0;
uint8 encoder_store = 0;

int8 read_encoder()
{
    uint8 a = 0;
    if(!ROTA_BIT) {
        a |= 1;
    }
    if(!ROTB_BIT) {
        a |= 2;
    }

    encoder_state <<= 2;
    encoder_state |= a;
    encoder_state &= 0xf;

    if(encoder_valid_bits[encoder_state] != 0) {
        encoder_store = (encoder_store << 4) | encoder_state;
        switch(encoder_store) {
        case 0xe8:
            return 1;
        case 0x2b:
            return -1;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////
// BOOTLOADER admin

typedef void (*BOOTLOADER)(void);
#define bootloader554 ((BOOTLOADER)0x3800)    // CH551/2/3/4
#define bootloader559 ((BOOTLOADER)0xF400)    // CH558/9

#define BOOTLOADER_DELAY 0x300    // about 3 seconds

//////////////////////////////////////////////////////////////////////
// Flash LED before jumping to bootloader

void bootloader_led_flash(int8_t n)
{
    LED_BIT = 0;
    for(int8_t i = 0; i < n; ++i) {
        TF2 = 0;
        TH2 = 0;
        TL2 = 120;
        while(TF2 != 1) {
        }
        LED_BIT ^= 1;
    }
}

//////////////////////////////////////////////////////////////////////
// read bytes from the data flash area

void read_flash_data(uint8 flash_addr, uint8 len, uint8 *data)
{
    flash_addr <<= 1;

    while(len-- != 0) {

        ROM_ADDR_L = flash_addr;
        ROM_ADDR_H = DATA_FLASH_ADDR >> 8;

        ROM_CTRL = ROM_CMD_READ;

        if((ROM_STATUS & bROM_CMD_ERR) != 0) {
            break;
        }

        *data++ = ROM_DATA_L;

        flash_addr += 2;
    }
}

//////////////////////////////////////////////////////////////////////
// write bytes to the data flash area

void write_flash_data(uint8 flash_addr, uint8 len, uint8 *data)
{
    flash_addr <<= 1;

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;

    GLOBAL_CFG |= bDATA_WE;

    while(len-- != 0) {

        ROM_ADDR_L = flash_addr;
        ROM_ADDR_H = DATA_FLASH_ADDR >> 8;

        ROM_DATA_L = *data++;

        ROM_CTRL = ROM_CMD_WRITE;

        if((ROM_STATUS & bROM_CMD_ERR) != 0) {
            break;
        }

        flash_addr += 2;
    }

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;

    GLOBAL_CFG &= ~bDATA_WE;
}

// Main function
int main()
{
    uint16_t press_time = 0;

    CfgFsys();       // CH559 clock select configuration
    mDelaymS(5);     // Modify the main frequency and wait for the internal crystal stability, it will be added
    UART0_Init();    // Candidate 0, can be used for debugging

    // set GPIO 1.6 as output push/pull

    P1_MOD_OC = 0b00000000;
    P1_DIR_PU = 0b01000000;

    // set GPIO 3.3 as bidirectional (button)
    // set GPIO 3.1 as bidirectional (rx)
    // set GPIO 3.0 as output push/pull (tx)

    P3_MOD_OC = 0b00011110;
    P3_DIR_PU = 0b00011111;

    USBDeviceCfg();
    USBDeviceEndPointCfg();    // Endpoint configuration
    USBDeviceIntCfg();         // Interrupt initialization
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;    // Be pre -use and sending length must be empty
    UEP2_T_LEN = 0;    // Be pre -use and sending length must be empty

#define DEBOUNCE_TIME 0xA0u
#define T2_DEBOUNCE (0xFFu - DEBOUNCE_TIME)

    putstr("Hello\n");

    TR0 = 1;
    TR2 = 1;

    uint8_t i = 0;
    while(i < 6) {
        if(TF2 == 1) {
            TL2 = 0;
            TH2 = 0;
            TF2 = 0;
            LED_BIT = !LED_BIT;
            i += 1;
        }
    }

    LED_BIT = 0;

    bool button_state = false;    // for debouncing the button

    read_flash_data(0, 1, &vol_direction);

    if(vol_direction == 0xff) {
        vol_direction = ROTARY_DIRECTION;
    }
    turn_value = (int8)vol_direction - 1;    // becomes -1 or 1

    while(1) {

        // read/debounce the button
        bool pressed = false;
        bool new_state = !BTN_BIT;

        if(new_state != button_state && TF2) {
            TL2 = 0;
            TH2 = T2_DEBOUNCE;
            TF2 = 0;
            pressed = new_state;
            button_state = new_state;
        }

        if(button_state) {
            if(TF0 == 1) {
                TF0 = 0;
                press_time += 1;
                if(press_time == BOOTLOADER_DELAY) {

                    // shutdown peripherals
                    EA = 0;
                    USB_CTRL = 0;
                    UDEV_CTRL = 0;

                    // flash LED for a bit
                    bootloader_led_flash(8);

                    // and jump to bootloader
                    bootloader554();
                }
            }
        } else {
            press_time = 0;
        }

        // read the rotary encoder (returns -1, 0 or 1)
        int8 direction = read_encoder();

        // Timer2 does double duty, debounces encoder as well
        if(direction != 0 && TF2 == 1) {
            TL2 = 0;
            TH2 = T2_DEBOUNCE;
            TF2 = 0;
        } else {
            direction = 0;
        }

        if(UsbConfig) {
            if(ep2_recv_len) {

                memcpy(midi_in_buffer, Ep2Buffer, ep2_recv_len);
                UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
                uint8_t length = ep2_recv_len;
                ep2_recv_len = 0;

                hexdump("raw", midi_in_buffer, length);

                process_midi_packet(length);
            }

            if(!ep2_busy) {
                if(pressed) {
                    Ep2Buffer[MAX_PACKET_SIZE + 0] = 0x0B;
                    Ep2Buffer[MAX_PACKET_SIZE + 1] = 0xB0;
                    Ep2Buffer[MAX_PACKET_SIZE + 2] = 0x03;
                    Ep2Buffer[MAX_PACKET_SIZE + 3] = 0x01;
                    UEP2_T_LEN = 4;
                    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;    // Answer ACK
                    ep2_busy = 1;
                    LED_BIT = !LED_BIT;
                }
                if(direction == turn_value) {
                    Ep2Buffer[MAX_PACKET_SIZE + 0] = 0x0B;
                    Ep2Buffer[MAX_PACKET_SIZE + 1] = 0xB0;
                    Ep2Buffer[MAX_PACKET_SIZE + 2] = 0x04;
                    Ep2Buffer[MAX_PACKET_SIZE + 3] = 0x01;
                    UEP2_T_LEN = 4;
                    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;    // Answer ACK
                    ep2_busy = 1;
                    LED_BIT = !LED_BIT;
                } else if(direction == -turn_value) {
                    Ep2Buffer[MAX_PACKET_SIZE + 0] = 0x0B;
                    Ep2Buffer[MAX_PACKET_SIZE + 1] = 0xB0;
                    Ep2Buffer[MAX_PACKET_SIZE + 2] = 0x05;
                    Ep2Buffer[MAX_PACKET_SIZE + 3] = 0x01;
                    UEP2_T_LEN = 4;
                    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;    // Answer ACK
                    ep2_busy = 1;
                    LED_BIT = !LED_BIT;
                }
            }
        }
    }
}
