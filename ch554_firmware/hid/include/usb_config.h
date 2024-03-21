#pragma once

//////////////////////////////////////////////////////////////////////
// USB config

#define USB_VID 0x16D0
#define USB_PID 0x114B

#define CUSTOM_PACKET_SIZE 32

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

// CUSTOM HID DEVICE

__code const uint8 custom_rep_desc[] = {

    0x06, 0x00, 0xff,    // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,          // USAGE (Vendor Usage 1)
    0xa1, 0x01,          // COLLECTION (Application)
    0x15, 0x00,          //   LOGICAL_MINIMUM 0
    0x25, 0xff,          //   LOGICAL_MAXIMUM 255
    0x75, 0x08,          //   REPORT_SIZE 8 bits
    0x95, 32,            //   REPORT_COUNT 32
    0x09, 0x01,          //   USAGE (Vendor Usage 1)
    0x81, 0x02,          //   INPUT (Data,Var,Abs)
    0x95, 32,            //   REPORT_COUNT 32
    0x09, 0x01,          //   USAGE (Vendor Usage 1)
    0x91, 0x02,          //   OUTPUT (Data,Var,Abs)
    0xc0                 // END_COLLECTION
};

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
    USB_VID & 0xff,          // idVendor (LSB)
    USB_VID >> 8,            // idVendor (MSB)
    USB_PID & 0xff,          // idProduct (LSB)
    USB_PID >> 8,            // idProduct (MSB)
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
    0x03,                              // bNumInterface
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

    // HID KEYBOARD
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

    // HID MEDIA CONTROL DEVICE
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
    0x02,                      // bInterval

    // Interface
    0x09,                    // bLength
    USB_DESCR_TYP_INTERF,    // bDescriptorType
    0x02,                    // bInterfaceNumber
    0x00,                    // bAlternateSetting
    0x02,                    // bNumEndpoints
    USB_DEV_CLASS_HID,       // bInterfaceClass
    0x00,                    // bInterfaceSubClass: none (no boot)
    0x00,                    // bInterfaceProtocol: none
    0x02,                    // iInterface

    // HID
    0x09,                              // bLength
    USB_DESCR_TYP_HID,                 // bDescriptorType: HID
    0x11,                              // bcdHID(1)
    0x01,                              // bcdHID(2)
    0x00,                              // bCountryCode
    0x01,                              // bNumDescriptors
    0x22,                              // bDescriptorType: Report
    sizeof(custom_rep_desc) & 0xff,    // wDescriptorLength (1)
    sizeof(custom_rep_desc) >> 8,      // wDescriptorLength (2)

    // Endpoint
    0x07,                         // bLength
    USB_DESCR_TYP_ENDP,           // bDescriptorType: ENDPOINT
    0x83,                         // bEndpointAddress: IN/Endpoint3
    USB_ENDP_TYPE_INTER,          // bmAttributes: Interrupt
    CUSTOM_PACKET_SIZE & 0xff,    // wMaxPacketSize (1)
    CUSTOM_PACKET_SIZE >> 8,      // wMaxPacketSize (2)
    0x02,                         // bInterval

    // Endpoint
    0x07,                         // bLength
    USB_DESCR_TYP_ENDP,           // bDescriptorType: ENDPOINT
    0x03,                         // bEndpointAddress: OUT/Endpoint3
    USB_ENDP_TYPE_INTER,          // bmAttributes: Interrupt
    CUSTOM_PACKET_SIZE & 0xff,    // wMaxPacketSize (1)
    CUSTOM_PACKET_SIZE >> 8,      // wMaxPacketSize (2)
    0x02                          // bInterval
};

//////////////////////////////////////////////////////////////////////
// UINT16s in here...

#define LANGUAGE_DESC 0x0409
#define MANUFACTURER_STRING 'T', 'i', 'n', 'y', ' ', 'L', 'i', 't', 't', 'l', 'e', ' ', 'T', 'h', 'i', 'n', 'g', 's'
#define KEYBOARD_STRING 'T', 'i', 'n', 'y', ' ', 'U', 'S', 'B', ' ', 'K', 'n', 'o', 'b', ' ', '(', 'K', 'B', 'D', ')'
#define MEDIA_STRING 'T', 'i', 'n', 'y', ' ', 'U', 'S', 'B', ' ', 'K', 'n', 'o', 'b', ' ', '(', 'C', 'C', ')'
#define CONFIG_STRING 'C', 'o', 'n', 'f', 'i', 'g'
#define CUSTOM_STRING 'T', 'i', 'n', 'y', ' ', 'U', 'S', 'B', ' ', 'K', 'n', 'o', 'b'

#define STR_HDR(x) (sizeof(x) | (USB_DESCR_TYP_STRING << 8))

__code const uint16 language_desc[] = { STR_HDR(language_desc), LANGUAGE_DESC };
__code const uint16 manufacturer_string[] = { STR_HDR(manufacturer_string), MANUFACTURER_STRING };
__code const uint16 keyboard_string[] = { STR_HDR(keyboard_string), KEYBOARD_STRING };
__code const uint16 media_string[] = { STR_HDR(media_string), MEDIA_STRING };
__code const uint16 config_string[] = { STR_HDR(config_string), CONFIG_STRING };
__code const uint16 custom_string[] = { STR_HDR(custom_string), CUSTOM_STRING };

//////////////////////////////////////////////////////////////////////

__code const usb_descriptor_t config_descs[] = {
    DESCRIPTOR(hid_config_desc)    // USB config
};

//////////////////////////////////////////////////////////////////////

__code const usb_descriptor_t report_descs[] = {
    DESCRIPTOR(keyboard_rep_desc),    // keyboard HID report
    DESCRIPTOR(media_rep_desc),       // media key HID report
    DESCRIPTOR(custom_rep_desc)       // custom report
};

//////////////////////////////////////////////////////////////////////

__code const usb_descriptor_t string_descs[] = {
    DESCRIPTOR(language_desc),           // 0
    DESCRIPTOR(manufacturer_string),     // 1
    DESCRIPTOR(product_name_string),     // 2 - dynamic in xdata
    DESCRIPTOR(serial_number_string),    // 3 - dynamic in xdata
    DESCRIPTOR(keyboard_string),         // 4
    DESCRIPTOR(media_string),            // 5
    DESCRIPTOR(config_string),           // 6
    DESCRIPTOR(custom_string)            // 7
};

//////////////////////////////////////////////////////////////////////

#define NUM_CONFIG_DESCS ARRAY_COUNT(config_descs)
#define NUM_REPORT_DESCS ARRAY_COUNT(report_descs)
#define NUM_STRING_DESCS ARRAY_COUNT(string_descs)
