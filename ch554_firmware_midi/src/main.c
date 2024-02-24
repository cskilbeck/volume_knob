//////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ch554.h>
#include <ch554_usb.h>
#include <debug.h>
#include "util.h"

#pragma disable_warning 110

//////////////////////////////////////////////////////////////////////
// BOOTLOADER admin

typedef void (*BOOTLOADER)(void);
#define bootloader554 ((BOOTLOADER)0x3800)    // CH551/2/3/4
#define bootloader559 ((BOOTLOADER)0xF400)    // CH558/9

#define BOOTLOADER_DELAY 0x300    // about 3 seconds

volatile const __code __at(ROM_CHIP_ID_LO)
uint16_t CHIP_UNIQUE_ID_LO;

volatile const __code __at(ROM_CHIP_ID_HI)
uint16_t CHIP_UNIQUE_ID_HI;

uint32 chip_id;

//////////////////////////////////////////////////////////////////////
// GPIO

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

//////////////////////////////////////////////////////////////////////
// Rotary Encoder

uint8 vol_direction;
int8 turn_value;

#define CLOCKWISE 2
#define ANTI_CLOCKWISE 0

// Define ROTARY_DIRECTION as CLOCKWISE for one kind of encoders, ANTI_CLOCKWISE for
// the other ones (some are reversed). This sets the default rotation (after first
// flash), reverse by triple-clicking the knob

#define ROTARY_DIRECTION (CLOCKWISE)
// #define ROTARY_DIRECTION (ANTI_CLOCKWISE)

//////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////

__xdata __at(0x0000) uint8 Ep0Buffer[DEFAULT_ENDP0_SIZE];     // endpoint0 OUT & IN buffer，Must be an even address
__xdata __at(0x0040) uint8 Ep1Buffer[DEFAULT_ENDP1_SIZE];     // endpoint1 upload buffer
__xdata __at(0x0080) uint8 Ep2Buffer[2 * MAX_PACKET_SIZE];    // endpoint2 IN & OUT buffer, Must be an even address

//////////////////////////////////////////////////////////////////////
// VID = 16D0, PID = 1317

#define USB_VID 0x16D0
#define USB_PID 0x1317

//////////////////////////////////////////////////////////////////////

uint16 SetupLen;
uint8 SetupReq;
uint8 UsbConfig;
const uint8 *pDescr;    // USB descriptor to send

#define UsbSetupBuf ((PUSB_SETUP_REQ)Ep0Buffer)

#define USB_DESC_TYPE_DEVICE 0x01

// Device descriptor
__code uint8 DevDesc[] = {
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

__code uint8 CfgDesc[] = {

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

#define MIDI_REV_LEN 32                        // MIDI receive buffer size
__idata uint8 midi_in_buffer[MIDI_REV_LEN];    // MIDI receive buffer

volatile __idata uint8 ep2_recv_len = 0;    // # received by USB endpoint
volatile __idata uint8 ep2_busy = 0;        // upload endpoint busy flag

//////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////

void USBDeviceIntCfg()
{
    USB_INT_EN |= bUIE_SUSPEND;     // Make the device hang up and interrupt
    USB_INT_EN |= bUIE_TRANSFER;    // Make USB transmission complete interruption
    USB_INT_EN |= bUIE_BUS_RST;     // Make the device mode USB bus reset and interrupt
    USB_INT_FG |= 0x1F;             // Clear interrupt
    IE_USB = 1;                     // Enable USB interrupt
    EA = 1;                         // Enable interrupts
}

//////////////////////////////////////////////////////////////////////

void USBDeviceEndPointCfg()
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

void DeviceInterrupt(void) __interrupt(INT_NO_USB)    // USB interrupt service program, use the register group 1
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
                SetupLen = ((uint16)UsbSetupBuf->wLengthH << 8) | (UsbSetupBuf->wLengthL);
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
                            if((((uint16)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01) {
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
                            if((((uint16)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01) {
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
                            if((((uint16)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x00) {
                                switch(((uint16)UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL) {
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
// a queue of MIDI packets
// we have to queue things up because the rotary encoder can generate
// messages faster than we can send them

typedef uint32 midi_packet;

// must be a power of 2
#define KEY_QUEUE_LEN 4

__idata midi_packet queue_buffer[KEY_QUEUE_LEN];
uint8 queue_head = 0;
uint8 queue_size = 0;

inline bool queue_full()
{
    return queue_size == KEY_QUEUE_LEN;
}

inline uint8 queue_space()
{
    return KEY_QUEUE_LEN - queue_size;
}

inline bool queue_empty()
{
    return queue_size == 0;
}

// push one onto the queue, check it's got space before calling this

void queue_put(midi_packet k)
{
    queue_buffer[(queue_head + queue_size) & (KEY_QUEUE_LEN - 1)] = k;
    queue_size += 1;
}

// pop next from the queue, check it's not empty before calling this

midi_packet queue_get()
{
    uint8 old_head = queue_head;
    queue_size -= 1;
    queue_head = ++queue_head & (KEY_QUEUE_LEN - 1);
    return queue_buffer[old_head];
}

// pop next into somewhere, check it's not empty before calling this

inline void queue_get_at(midi_packet *dst)
{
    *dst = queue_buffer[queue_head];
    queue_size -= 1;
    queue_head = ++queue_head & (KEY_QUEUE_LEN - 1);
}

//////////////////////////////////////////////////////////////////////
// send the next waiting packet if there is one and the usb is ready

void midi_packet_send_update()
{
    if(!ep2_busy && !queue_empty()) {
        queue_get_at((uint32 *)(Ep2Buffer + MAX_PACKET_SIZE));
        hexdump("send", Ep2Buffer + MAX_PACKET_SIZE, 4);
        UEP2_T_LEN = 4;
        UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;    // Answer ACK
        ep2_busy = 1;
    }
}

//////////////////////////////////////////////////////////////////////

#define MIDI_MANUFACTURER_ID 0x36    // Cheetah Marketing, defunct
#define MIDI_FAMILY_CODE_LOW 0x55
#define MIDI_FAMILY_CODE_HIGH 0x44
#define MIDI_MODEL_NUMBER_LOW 0x33
#define MIDI_MODEL_NUMBER_HIGH 0x22

//////////////////////////////////////////////////////////////////////

__code uint8 const identity_request_2[] = { 0xF0, 0x7E };
// channel in between, we ignore it
__code uint8 const identity_request_3[] = { 0x06, 0x01, 0xF7 };

#define IDENTITY_REQUEST_LENGTH (sizeof(identity_request_2) + 1 + sizeof(identity_request_3))

__idata uint8 send_buffer[16];

uint8 const identity_response[] = {
    0xF0,    // identity response header
    0x7E,    // identity response header
    0x00,    // identity response header
    0x06,    // identity response header
    0x02,    // identity response header
    MIDI_MANUFACTURER_ID,
    MIDI_FAMILY_CODE_LOW,
    MIDI_FAMILY_CODE_HIGH,
    MIDI_MODEL_NUMBER_LOW,
    MIDI_MODEL_NUMBER_HIGH,
    0x00,    // UNIQUE_ID goes in here
    0x00,    // UNIQUE_ID goes in here
    0x00,    // UNIQUE_ID goes in here
    0x00,    // UNIQUE_ID goes in here
    0xF7     // sysex terminator
};

//////////////////////////////////////////////////////////////////////

uint8 *usb_send_ptr;
uint8 usb_sent = 0;
uint8 usb_send_remain = 0;

bool usb_send_update()
{
    uint8 r = usb_send_remain;
    if(r == 0) {
        return false;
    }

    if(r > 3) {
        r = 3;
    }
    uint8 cmd = mci_sysex_start;
    if(r < 3 || usb_send_remain <= 3) {
        cmd = mci_sysex_end_1 + r - 1;
    }
    midi_packet packet = 0;
    uint8 *dst = (uint8 *)&packet;
    uint8 *src = usb_send_ptr;
    *dst++ = cmd;
    for(uint8 i = 0; i < r; ++i) {
        *dst++ = *src++;
    }
    queue_put(packet);
    usb_sent += r;
    usb_send_ptr += r;
    usb_send_remain -= r;
    return true;
}

typedef struct config
{
    uint8 flags;
    uint8 rotate_delta;
    uint8 rotate_cc;
    uint8 click_cc;
    uint8 click_press_value;
    uint8 click_release_value;

} config_t;

#define FLAG_REL_ABS 1
#define FLAG_REVERSE 2
#define FLAG_LED_PRESS 4
#define FLAG_LED_RELEASE 8
#define FLAG_LED_ROTATE 16
#define FLAG_ACCEL_0 32
#define FLAG_ACCEL_1 64


//////////////////////////////////////////////////////////////////////

bool usb_send(uint8 *data, uint8 length)
{
    if(usb_send_remain != 0) {
        return false;
    }
    if(length < 4) {
        return false;
    }
    usb_send_remain = length;
    usb_send_ptr = data;
    return true;
}

//////////////////////////////////////////////////////////////////////

__idata uint8 sysex_recv_buffer[16];
__idata uint8 sysex_recv_length = 0;
__idata uint8 sysex_recv_packet_offset = 0;
__idata uint8 device_id = 0;

void sysex_parse_add(uint8 length)
{
    if(length > (sizeof(sysex_recv_buffer) - sysex_recv_length)) {
        sysex_recv_length = 0;
    } else {
        memcpy(sysex_recv_buffer + sysex_recv_length, midi_in_buffer + sysex_recv_packet_offset + 1, length);
        sysex_recv_length += length;

        if(sysex_recv_buffer[sysex_recv_length - 1] == 0xF7) {

            hexdump("sysex", sysex_recv_buffer, sysex_recv_length);

            sysex_recv_length = 0;

            if(sysex_recv_buffer[0] == 0xF0 && sysex_recv_buffer[1] == 0x7E && sysex_recv_buffer[3] == 0x06) {

                switch(sysex_recv_buffer[4]) {

                // identity request
                case 0x01:
                    device_id = sysex_recv_buffer[2];
                    memcpy(send_buffer, identity_response, sizeof(identity_response));
                    send_buffer[2] = device_id;
                    send_buffer[10] = (chip_id >> 21) & 0x7f;
                    send_buffer[11] = (chip_id >> 14) & 0x7f;
                    send_buffer[12] = (chip_id >> 7) & 0x7f;
                    send_buffer[13] = (chip_id >> 0) & 0x7f;
                    usb_send(send_buffer, sizeof(identity_response));
                    break;

                // toggle led
                case 0x02:
                    LED_BIT = !LED_BIT;
                    break;

                // Get flash
                case 0x03: {
                    putstr("Get flash!?\n");
                    uint8 buffer[8];
                    read_flash_data(0, 8, buffer);
                    send_buffer[2] = device_id;
                    send_buffer[4] = 0x10;
                    memset(send_buffer + 5, 0, 10);
                    bytes_to_bits7(buffer, 0, 8, send_buffer + 5);
                    send_buffer[15] = 0xf7;
                    usb_send(send_buffer, 16);
                } break;

                // Set flash
                case 0x04: {
                    uint8 buffer[8];
                    bits7_to_bytes(sysex_recv_buffer, 5, 8, buffer);
                    hexdump("WRITE", buffer, 8);
                    write_flash_data(0, 8, buffer);
                    send_buffer[2] = device_id;
                    send_buffer[4] = 0x11;
                    send_buffer[5] = 0xF7;
                } break;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////

void process_midi_packet_in(uint8 length)
{
    sysex_recv_packet_offset = 0;

    while(sysex_recv_packet_offset < length) {

        uint8 cmd = midi_in_buffer[sysex_recv_packet_offset];

        switch(cmd) {

        case mci_sysex_start:
        case mci_sysex_end_3:
            sysex_parse_add(3);
            break;

        case mci_sysex_end_1:
            sysex_parse_add(1);
            break;
        case mci_sysex_end_2:
            sysex_parse_add(2);
            break;

        default:
            sysex_recv_length = 0;
            sysex_recv_packet_offset = length;
            break;
        }
        sysex_recv_packet_offset += 4;
    }
}

//////////////////////////////////////////////////////////////////////
// rotary encoder reader

__code const uint8 encoder_valid_bits[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

__idata uint8 encoder_state = 0;
__idata uint8 encoder_store = 0;

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

int main()
{
    uint16_t press_time = 0;

    CfgFsys();       // CH559 clock select configuration
    mDelaymS(5);     // Modify the main frequency and wait for the internal crystal stability, it will be added
    UART0_Init();    // Candidate 0, can be used for debugging

    chip_id = CHIP_UNIQUE_ID_LO | ((uint32)CHIP_UNIQUE_ID_HI << 16);

    hexdump("CHIPID", &chip_id, 4);

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

    if(!read_flash_data(DATA_FLASH_ADDR, 1, &vol_direction)) {
        putstr("Error reading flash data\n");
    } else {
        hexdump("Vol Direction", &vol_direction, 1);
    }

    switch(vol_direction) {
    case 0:
        turn_value = -1;
        break;
    case 2:
        turn_value = 1;
        break;
    default:
        turn_value = ROTARY_DIRECTION - 1;
        break;
    }

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

                uint8_t length = ep2_recv_len;
                if(length > sizeof(midi_in_buffer)) {
                    length = sizeof(midi_in_buffer);
                }

                memcpy(midi_in_buffer, Ep2Buffer, ep2_recv_len);
                UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
                ep2_recv_len = 0;

                hexdump("raw", midi_in_buffer, length);

                process_midi_packet_in(length);
            }

            // send any waiting midi packets
            midi_packet_send_update();

            // maybe add some more to the queue
            if(!queue_full() && !usb_send_update()) {

                if(pressed) {

                    queue_put(0x0103b00b);
                    LED_BIT = !LED_BIT;
                }
                if(!queue_full()) {
                    if(direction == turn_value) {

                        queue_put(0x0104b00b);
                        LED_BIT = !LED_BIT;

                    } else if(direction == -turn_value) {

                        queue_put(0x0105b00b);
                        LED_BIT = !LED_BIT;
                    }
                }
            }
        }
    }
}
