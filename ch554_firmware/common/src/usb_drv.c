#include "main.h"

//////////////////////////////////////////////////////////////////////

#if defined(DEBUG)
// #define USB_LOGGING
#endif

#if defined(USB_LOGGING)
#define usb_printf printf
#define usb_puts puts
#else
#define usb_printf(...) NOP_MACRO
#define usb_puts(...) NOP_MACRO
#endif

//////////////////////////////////////////////////////////////////////

#define usb_setup ((USB_SETUP_REQ *)usb_endpoint_0_buffer)

void usb_isr(void) __interrupt(INT_NO_USB)
{
    uint8 len;

    if(UIF_TRANSFER) {

        switch(USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) {

        // setup request on interface 0
        case UIS_TOKEN_SETUP | 0:

            len = 0xff;

            // should be this much, anything else seems to be an error
            if(USB_RX_LEN == sizeof(USB_SETUP_REQ)) {

                // how much it wants, capped at 127
                usb.setup_len = MIN(127, U16(usb_setup->wLengthL));

                len = 0;
                usb.setup_request = usb_setup->bRequest;
                uint8 request_type = usb_setup->bRequestType;

                // all request types except 'standard' are ignored
                switch(request_type & USB_REQ_TYP_MASK) {

                case USB_REQ_TYP_STANDARD:

                    switch(usb.setup_request) {

                    case USB_GET_DESCRIPTOR:

                        switch(usb_setup->wValueH) {

                        case USB_DESCR_TYP_DEVICE:
                            usb_puts("USB:GetDevice");
                            usb.current_descriptor = usb_cfg.device_descriptor->p;
                            len = usb_cfg.device_descriptor->len;
                            break;

                        case USB_DESCR_TYP_CONFIG: {
                            uint8 desc = usb_setup->wValueL;
                            usb_printf("USB:GetConfig %d\n", desc);
                            len = 0xff;
                            if(desc < usb_cfg.num_config_descriptors) {
                                usb.current_config_desc = usb_cfg.config_descriptors[desc].p;
                                usb.current_descriptor = usb.current_config_desc;
                                len = usb_cfg.config_descriptors[desc].len;
                            }
                        } break;

                        case USB_DESCR_TYP_STRING: {
                            len = 0xff;
                            uint8 desc = usb_setup->wValueL;
                            usb_printf("USB:GetString %d\n", desc);
                            if(desc < usb_cfg.num_string_descriptors) {
                                usb.current_descriptor = usb_cfg.string_descriptors[desc].p;
                                len = usb_cfg.string_descriptors[desc].len;
                            }
                        } break;

                        case USB_DESCR_TYP_REPORT: {
                            len = 0xff;
                            uint8 desc = usb_setup->wIndexL;    // !! Index!? I guess...
                            usb_printf("USB:GetReport %d\n", desc);
                            if(desc < usb_cfg.num_report_descriptors) {
                                usb.current_descriptor = usb_cfg.report_descriptors[desc].p;
                                len = usb_cfg.report_descriptors[desc].len;
                            }
                        } break;

                        default:
                            len = 0xff;
                            break;
                        }

                        // continuation?
                        if(usb.setup_len > len) {
                            usb.setup_len = len;
                        }

                        // send at most ? bytes per request (coalesced on host)
                        len = MIN(usb.setup_len, USB_PACKET_SIZE);
                        memcpy(usb_endpoint_0_buffer, usb.current_descriptor, len);
                        usb.setup_len -= len;
                        usb.current_descriptor += len;
                        usb_printf("Start send %d\n", len);

                        // remainder will get sent in subsequent packets
                        break;

                    case USB_SET_ADDRESS:
                        usb_puts("SetAddr");
                        usb.setup_len = usb_setup->wValueL;
                        break;

                    case USB_GET_CONFIGURATION:
                        usb_puts("GetConfig");
                        usb_endpoint_0_buffer[0] = usb.config;
                        usb.setup_len = MIN(usb.setup_len, 1);
                        break;

                    case USB_SET_CONFIGURATION:
                        usb_puts("SetConfig");
                        usb.config = usb_setup->wValueL;
                        usb.active = true;
                        break;

                    case USB_SET_INTERFACE:
                        usb_puts("SetInterface...?");
                        break;

                    case USB_GET_INTERFACE:
                        usb_puts("GetInterface...?");
                        break;

                    case USB_CLEAR_FEATURE:

                        switch(usb_setup->bRequestType & 0x1F) {

                        case USB_REQ_RECIP_DEVICE:    // Device

                            if(U16(usb_setup->wValueL) == 0x01) {
                                if((usb.current_config_desc[7] & 0x20) != 0) {
                                    usb_puts("Wake");
                                } else {
                                    usb_puts("Wake not supported");
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
                            if(U16(usb_setup->wValueL) == 0x0001) {

                                // asking to set remote wakeup!?
                                if((usb.current_config_desc[7] & 0x20) != 0) {
                                    // len = 0;
                                } else {
                                    len = 0xff;
                                }
                            } else {
                                len = 0xff;
                            }
                        } break;

                        case USB_REQ_RECIP_ENDP: {

                            if(U16(usb_setup->wValueL) == 0x0000) {
                                switch(U16(usb_setup->wIndexL)) {
                                case 0x83:
                                    UEP3_CTRL = UEP3_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;
                                    break;
                                case 0x03:
                                    UEP3_CTRL = UEP3_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;
                                    break;
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;
                                    break;
                                case 0x01:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;
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
                        usb_endpoint_0_buffer[0] = 0x00;    // bus powered
                        usb_endpoint_0_buffer[1] = 0x00;    // no remote wakeup
                        len = MIN(usb.setup_len, 2);
                        break;

                    default:
                        len = 0xff;
                        break;
                    }
                    break;

                default:

                    switch(usb.setup_request) {

                    case HID_SET_REPORT:
                        usb_printf("Set report\n");
                        len = 0;
                        break;

                    case HID_SET_IDLE:
                        usb_printf("Set Idle\n");
                        len = 0;
                        break;

                    case HID_GET_REPORT:
                    case HID_GET_IDLE:
                    case HID_GET_PROTOCOL:
                    case HID_SET_PROTOCOL:
                        usb_printf("OK Setup req: %d\n", usb.setup_request);
                        len = 0;
                        break;

                    default:
                        usb_printf("NOT OK Setup req: %d\n", usb.setup_request);
                        len = 0xff;
                        break;
                    }
                    break;
                }
            }


            if(len == 0xff) {    // if len == 0xff, some error has occurred

                usb_puts("Err - stalling");
                usb.setup_request = 0xff;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;    // set the data toggles to 1 and indicate stall

            } else if(len <= USB_PACKET_SIZE) {    // if len <= 8, some valid data is sitting in EP0Buffer (or 0 bytes, that's ok)

                UEP0_T_LEN = len;                                                       // send if there is any
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;    // ack and toggle
            }
            break;

        // ENDPOINT 0 transmit to host complete
        case UIS_TOKEN_IN | 0:

            // what were we doing to cause data to be sent?
            switch(usb.setup_request) {

            // continue to send existing stuff(based on usb.setup_request)
            case USB_GET_DESCRIPTOR:
                len = MIN(usb.setup_len, USB_PACKET_SIZE);
                if(len != 0) {
                    usb_printf(" more send %d\n", len);
                    memcpy(usb_endpoint_0_buffer, usb.current_descriptor, len);
                    usb.setup_len -= len;
                    usb.current_descriptor += len;
                }
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;    // ep0 manual toggle
                break;

            // usb.setup_len was hijacked for device address!!!
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | usb.setup_len;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;

            // else... huh?
            default:
                UEP0_T_LEN = 0;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;

        // ENDPOINT 0 receive from host complete, just ACK it
        case UIS_TOKEN_OUT | 0:
            UEP0_T_LEN = 0;
            UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_ACK;
            break;

        // ENDPOINT 1 transmit to host complete
        case UIS_TOKEN_IN | 1:
            UEP1_T_LEN = 0;
            UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            usb.idle |= 1;
            break;

        // ENDPOINT 1 receive from host complete
        case UIS_TOKEN_OUT | 1:
            if(U_TOG_OK) {
                usb.recv_len[1] = USB_RX_LEN;
                UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
            }
            break;

        // ENDPOINT 2 transmit to host complete
        case UIS_TOKEN_IN | 2:
            UEP2_T_LEN = 0;
            UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            usb.idle |= 2;
            break;

        // ENDPOINT 2 receive from host complete
        case UIS_TOKEN_OUT | 2:
            if(U_TOG_OK) {
                usb.recv_len[2] = USB_RX_LEN;
                UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
            }
            break;

        // ENDPOINT 3 transmit to host complete
        case UIS_TOKEN_IN | 3:
            UEP3_T_LEN = 0;
            UEP3_CTRL = (UEP3_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            usb.idle |= 4;
            break;

        // ENDPOINT 3 receive from host complete
        case UIS_TOKEN_OUT | 3:
            if(U_TOG_OK) {
                usb.recv_len[3] = USB_RX_LEN;
                UEP3_CTRL = (UEP3_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
            }
            break;

        default:
            break;
        }
        UIF_TRANSFER = 0;
    }

    // bus reset
    if(UIF_BUS_RST) {
        usb_puts("USB Reset");
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
        UEP3_CTRL = bUEP_AUTO_TOG | UEP_T_RES_ACK | UEP_R_RES_ACK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        usb.config = 0;
        usb.idle = 7;
        usb.active = false;
    }

    // USB bus hangs/Wake up
    if(UIF_SUSPEND) {
        UIF_SUSPEND = 0;
        // Hang up
        if(USB_MIS_ST & bUMS_SUSPEND) {
            usb_puts("USB Suspend");
            while(XBUS_AUX & bUART0_TX) {    // Wait for msg to send
            }
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = bWAK_BY_USB;
            PCON |= PD;
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = 0x00;
        }
    }
    USB_INT_FG = 0x1F;    // clear all writeable USB IRQ flags
}

//////////////////////////////////////////////////////////////////////

static void usb_device_config()
{
    // USB device and internal pull-up enabled, automatically
    // return to NAK before the interrupt flag is cleared during the interrupt.
    USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;

    // device address 0 before host assigns it
    USB_DEV_AD = 0x00;

    // Disable DP/DM pull-downs, enable USB port
    UDEV_CTRL = bUD_PD_DIS | bUD_PORT_EN;
}

//////////////////////////////////////////////////////////////////////

static void usb_device_int_config()
{
    // suspend, transmission complete, bus reset IRQs enabled
    USB_INT_EN |= bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;

    // Clear outstanding interrupts
    USB_INT_FG |= 0x1F;

    // Enable USB interrupt
    IE_USB = 1;
}

//////////////////////////////////////////////////////////////////////

static void usb_device_endpoint_config()
{
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;
    UEP2_T_LEN = 0;
    UEP3_T_LEN = 0;

    UEP0_DMA = (uint16)usb_endpoint_0_buffer;
    UEP1_DMA = (uint16)usb_endpoint_1_rx_buffer;
    UEP2_DMA = (uint16)usb_endpoint_2_rx_buffer;
    UEP3_DMA = (uint16)usb_endpoint_3_rx_buffer;

    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;

    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
    UEP3_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;

    // EP4 disabled, EP1,2,3 enabled for TX,RX no double buffering
    UEP4_1_MOD = (uint8)(~(bUEP4_RX_EN | bUEP4_TX_EN | bUEP1_BUF_MOD) | bUEP1_RX_EN | bUEP1_TX_EN);
    UEP2_3_MOD = (uint8)(~(bUEP3_BUF_MOD | bUEP2_BUF_MOD) | bUEP2_RX_EN | bUEP2_TX_EN | bUEP3_RX_EN | bUEP3_TX_EN);

    usb.recv_len[0] = 0;    // this is a dummy - endpoint 0 is for config

    usb.recv_len[1] = 0;
    usb.recv_len[2] = 0;
    usb.recv_len[3] = 0;
}

//////////////////////////////////////////////////////////////////////

static void usb_init_strings()
{
    // insert chip id into serial string and product name string

    ASSERT(usb_cfg.product_name_length < (sizeof(product_name_string) / 2));

    uint8 name_prefix = usb_cfg.product_name_length;

    product_name_string[0] = (name_prefix + SERIAL_LEN + 1) * 2;
    product_name_string[1] = USB_DESCR_TYP_STRING;

    serial_number_string[0] = sizeof(serial_number_string);
    serial_number_string[1] = USB_DESCR_TYP_STRING;

    for(uint8 i = 0; i < name_prefix; ++i) {
        product_name_string[2 + i * 2] = usb_cfg.product_name[i];
        product_name_string[3 + i * 2] = 0;
    }

    product_name_string[name_prefix * 2 + 2] = ' ';
    product_name_string[name_prefix * 2 + 3] = 0;

    uint32 n = chip_id;

    for(uint8 i = 0; i < SERIAL_LEN; ++i) {

        uint8 c = (uint8)(n >> 28);

        if(c < 10) {
            c += '0';
        } else {
            c += 'A' - 10;
        }

        serial_number_string[2 + 2 * i] = c;
        serial_number_string[3 + 2 * i] = 0;

        product_name_string[2 + 2 * (i + name_prefix + 1)] = c;
        product_name_string[3 + 2 * (i + name_prefix + 1)] = 0;

        n <<= 4;
    }
}

//////////////////////////////////////////////////////////////////////
// flash led slowly until USB is connected (in case of power-only cable)

void usb_wait_for_connection()
{
    LED_BIT = LED_OFF;

    uint16 flash = 0;

    while(!usb.active) {

        tick_wait(1);

        flash += 1;
        if(flash == 1000) {
            flash = 0;
            LED_BIT ^= 1;
        }
    }
}

//////////////////////////////////////////////////////////////////////

void usb_send(usb_endpoint_t endpoint, uint8 len)
{
    uint8 mask = 0xff;
    switch(endpoint) {
    case endpoint_1:
        UEP1_T_LEN = len;
        UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
        mask = (uint8)(~(1 << 0));
        break;
    case endpoint_2:
        UEP2_T_LEN = len;
        UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
        mask = (uint8)(~(1 << 1));
        break;
    case endpoint_3:
        UEP3_T_LEN = len;
        UEP3_CTRL = (UEP3_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
        mask = (uint8)(~(1 << 2));
        break;
    case endpoint_4:
        UEP4_T_LEN = len;
        UEP4_CTRL = (UEP4_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
        mask = (uint8)(~(1 << 3));
        break;
    }
    usb.idle &= mask;
}

//////////////////////////////////////////////////////////////////////

bool usb_is_endpoint_idle(usb_endpoint_t endpoint)
{
    return (usb.idle & (1 << (endpoint - 1))) != 0;
}

//////////////////////////////////////////////////////////////////////

void usb_init()
{
    usb_init_strings();
    usb_device_config();
    usb_device_endpoint_config();
    usb_device_int_config();
}
