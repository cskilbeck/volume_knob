/********************************** (C) COPYRIGHT *******************************
 * File Name		: main.c
 * Author			: Zhiyuan Wan
 * License			: MIT
 * Version			: V1.0
 * Date				: 2018/03/27
 * Description		: CH55X is a USB-MIDI bridge.
 * 						Using UAAAC-MIDI protocol.
 * 						This example is a simple loop return device.
 *******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ch554.h>
#include <ch554_usb.h>
#include <debug.h>

__xdata __at(0x0000) uint8_t Ep0Buffer[DEFAULT_ENDP0_SIZE];     // endpoint0 OUT & IN buffer，Must be an even address
__xdata __at(0x0040) uint8_t Ep1Buffer[DEFAULT_ENDP1_SIZE];     // endpoint1 upload buffer
__xdata __at(0x0080) uint8_t Ep2Buffer[2 * MAX_PACKET_SIZE];    // endpoint2 IN & OUT buffer, Must be an even address

uint16_t SetupLen;
uint8_t SetupReq;
// uint8_t Count;
uint8_t UsbConfig;
const uint8_t *pDescr;        // USBConfiguration flags
USB_SETUP_REQ SetupReqBuf;    // temporary storage Setup Buffer

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
    0x86,                  // USB_VID (low)
    0x1a,                  // USB_VID (high)
    0x22,                  // USB_PID (low)
    0x57,                  // USB_PID (high)
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

#define MIDI_REV_LEN 64                            // Serial port receive buffer size
__idata uint8_t Receive_Midi_Buf[MIDI_REV_LEN];    // Serial port receive buffer
volatile __idata uint8_t Midi_Input_Point = 0;     // Circular buffer write pointer, bus reset needs to be initialized to 0
volatile __idata uint8_t Midi_Output_Point = 0;    // The circular buffer takes out the pointer and needs to be initialized to 0 for bus reset.
volatile __idata uint8_t MidiByteCount = 0;        // The number of bytes remaining in the current buffer to be fetched
volatile __idata uint8_t USBByteCount = 0;         // Represents data received by the USB endpoint
volatile __idata uint8_t USBBufOutPoint = 0;       // Get data pointer
volatile __idata uint8_t UpPoint2_Busy = 0;        // Whether the upload endpoint is busy flag

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
    USB_INT_EN |= bUIE_SUSPEND;     // 使能设备挂起中断
    USB_INT_EN |= bUIE_TRANSFER;    // 使能USB传输完成中断
    USB_INT_EN |= bUIE_BUS_RST;     // 使能设备模式USB总线复位中断
    USB_INT_FG |= 0x1F;             // 清中断标志
    IE_USB = 1;                     // 使能USB中断
    EA = 1;                         // 允许单片机中断
}
/*******************************************************************************
 * Function Name  : USBDeviceEndPointCfg()
 * Description	: USB设备模式端点配置，模拟兼容HID设备，除了端点0的控制传输，还包括端点2批量上下传
 * Input		  : None
 * Output		 : None
 * Return		 : None
 *******************************************************************************/
void USBDeviceEndPointCfg()
{
    UEP1_DMA = (uint16_t)Ep1Buffer;    // Point point 1 Send data transmission address
    UEP2_DMA = (uint16_t)Ep2Buffer;    // Writer 2 in data transmission address
    UEP2_3_MOD = 0xCC;                 // Endpoint 2/3 Single Single Single Receiving Fail
    UEP2_CTRL =
        bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;    // Writing 2 automatically flip the synchronous logo position, IN transaction returns NAK, out of ACK

    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;    // Point 1 automatically flip the synchronization logo bit, IN transaction returns NAK
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

void hexdump(uint8_t *p, uint8_t n)
{
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
    if(UIF_TRANSFER)    // USB transmission completion logo
    {
        switch(USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) {
        case UIS_TOKEN_IN | 1:    // endpoint 1# Endpoint interrupt upload
            UEP1_T_LEN = 0;
            UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;    // Answer NAK by default
            break;
        case UIS_TOKEN_IN | 2:    // endpoint 2# Endpoint batch upload
        {
            UEP2_T_LEN = 0;                                             // Be pre -use and sending length must be empty
            UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;    // Answer NAK by default
            UpPoint2_Busy = 0;                                          // Clear busy logo
        } break;
        case UIS_TOKEN_OUT | 2:    // endpoint 3# Endpoint batch passage
            if(U_TOG_OK)           // Discarded data packets will be discarded
            {
                USBByteCount = USB_RX_LEN;
                USBBufOutPoint = 0;                                         // Data pointer reset
                UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_NAK;    // After receiving a package of data, it is nak, the main function is processed, and
                                                                            // the main function modify the response method
            }
            break;
        case UIS_TOKEN_SETUP | 0:    // SETUPAffairs
            len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ))) {
                SetupLen = ((uint16_t)UsbSetupBuf->wLengthH << 8) | (UsbSetupBuf->wLengthL);
                len = 0;    // The default is success and upload 0 length
                SetupReq = UsbSetupBuf->bRequest;
                if((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)    // Non -standard request
                {
                    switch(SetupReq) {

                    default:
                        len = 0xFF;    // Command does not support
                        break;
                    }
                } else    // Standard request
                {
                    switch(SetupReq)    // Request code
                    {
                    case USB_GET_DESCRIPTOR:
                        switch(UsbSetupBuf->wValueH) {
                        case 1:                  // Device descriptor
                            pDescr = DevDesc;    // Send the device descriptor to the buffer to be sent
                            len = sizeof(DevDesc);
                            break;
                        case 2:                  // Configuration descriptor
                            pDescr = CfgDesc;    // Send the device descriptor to the buffer to be sent
                            len = sizeof(CfgDesc);
                            break;
                        case 3:
                            if(UsbSetupBuf->wValueL == 0) {
                                pDescr = language_string;
                                len = sizeof(language_string);
                            } else if(UsbSetupBuf->wValueL == 1) {
                                pDescr = manufacturer_string;
                                len = sizeof(manufacturer_string);
                            } else if(UsbSetupBuf->wValueL == 2) {
                                pDescr = product_string;
                                len = sizeof(product_string);
                            } else {
                                pDescr = serial_number_string;
                                len = sizeof(serial_number_string);
                            }
                            break;
                        default:
                            len = 0xff;    // Unwilling commands or errors
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
                        if((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_DEVICE)    // Removal
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
                                len = 0xFF;    // Unswerving endpoint
                                break;
                            }
                        } else {
                            len = 0xFF;    // Not the endpoint does not support
                        }
                        break;
                    case USB_SET_FEATURE:                                                 // Set Feature
                        if((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_DEVICE)    // Setting device
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
                UEP0_CTRL ^= bUEP_T_TOG;    // Synchronous logo flip
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
        case UIS_TOKEN_OUT | 0:    // endpoint0 OUT
                                   /*
                                   if(SetupReq ==SET_LINE_CODING)  //Set the serial attribute
                                   {
                                       if( U_TOG_OK )
                                       {
                                       //	memcpy(LineCoding,UsbSetupBuf,USB_RX_LEN);
                                       //	Config_Uart1(LineCoding);
                                           UEP0_T_LEN = 0;
                                           UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK;  // 准备上传0包
                                       }
                                   }
                                   else
                                   {
                                    */
            UEP0_T_LEN = 0;
            UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK;    // State stage, in response to NAK
            //}
            break;



        default:
            break;
        }
        UIF_TRANSFER = 0;    // Write 0 empty interrupt
    }
    if(UIF_BUS_RST)    // Equipment mode USB bus reset and interrupt
    {
        putstr("reset\n");    // Sleep state
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        UIF_BUS_RST = 0;          // Clear interruption logo
        Midi_Input_Point = 0;     // Circle buffer input pointer
        Midi_Output_Point = 0;    // Reading pointer in the circulating buffer
        MidiByteCount = 0;        // The remaining number of bytes to be taken in the current buffer
        USBByteCount = 0;         // The length received by USB endpoint
        UsbConfig = 0;            // Clear configuration value
        UpPoint2_Busy = 0;
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
    } else {                  // Unexpected interruption, the impossible situation
        USB_INT_FG = 0xFF;    // Clear interruption logo
    }
}

#define PORT1 0x90
#define PORT3 0xB0

#define BTN_PORT PORT3
#define BTN_PIN 3

#define LED_PORT PORT1
#define LED_PIN 6

// Set btn bit (P3.3) as input with pullup

SBIT(BTN_BIT, BTN_PORT, BTN_PIN);
SBIT(LED_BIT, LED_PORT, LED_PIN);

// Main function
int main()
{
    uint8_t length = 0;
    uint8_t Midi_Timeout = 0;
    CfgFsys();       // CH559 clock select configuration
    mDelaymS(5);     // Modify the main frequency and wait for the internal crystal stability, it will be added
    init_uart0();    // Candidate 0, can be used for debugging

    P1_MOD_OC = 0b00000000;
    P1_DIR_PU = 0b01000000;

    P3_MOD_OC = 0b00001010;
    P3_DIR_PU = 0b00001010;

#ifdef DE_PRINTF
    printf("start ...\r\n");
#endif

    USBDeviceCfg();
    USBDeviceEndPointCfg();    // Endpoint configuration
    USBDeviceIntCfg();         // Interrupt initialization
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;    // Be pre -use and sending length must be empty
    UEP2_T_LEN = 0;    // Be pre -use and sending length must be empty

#define DEBOUNCE_TIME 0xA0u
#define T2_DEBOUNCE (0xFFu - DEBOUNCE_TIME)

    putstr("Hello\n");

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

        if(UsbConfig) {
            if(USBByteCount) {
                memcpy(Receive_Midi_Buf, Ep2Buffer, USBByteCount);
                UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
                length = USBByteCount;
                USBByteCount = 0;

                hexdump(Receive_Midi_Buf, length);

                if(Receive_Midi_Buf[0] == 0x04 && Receive_Midi_Buf[1] == 0xF0 && Receive_Midi_Buf[2] == 0x7D && Receive_Midi_Buf[3] == 114) {
                    LED_BIT = !LED_BIT;
                }
            }

            if(!UpPoint2_Busy && pressed) {
                Ep2Buffer[MAX_PACKET_SIZE + 0] = 0x0B;
                Ep2Buffer[MAX_PACKET_SIZE + 1] = 0xB0;
                Ep2Buffer[MAX_PACKET_SIZE + 2] = 0x03;
                Ep2Buffer[MAX_PACKET_SIZE + 3] = 0x01;
                UEP2_T_LEN = 4;
                UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;    // Answer ACK
                UpPoint2_Busy = 1;
                length = 0;

                LED_BIT = !LED_BIT;
            }
        }
    }
}
