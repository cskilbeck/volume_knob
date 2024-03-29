#pragma once

#include "vs_lint.h"

//////////////////////////////////////////////////////////////////////
// USB config

#define USB_VID 0x16D0
#define USB_PID 0x114B

#define CUSTOM_PACKET_SIZE 32

//////////////////////////////////////////////////////////////////////

// KEYBOARD DEVICE

__code const uint8 hid_rep_desc[] = {

    0x05, 0x01,    // Usage Page: Generic Desktop Controls
    0x09, 0x06,    // Usage: Keyboard
    0xA1, 0x01,    // Collection: Application
    0x85, 0x01,    // REPORT_ID (1)

    // modifiers
    0x05, 0x07,    // Usage Page: Keyboard
    0x19, 0xE0,    // Usage Minimum: Keyboard LeftControl
    0x29, 0xE7,    // Usage Maximum: Keyboard Right GUI
    0x15, 0x00,    // Logical Minimum: 0
    0x25, 0x01,    // Logical Maximum: 1
    0x75, 0x01,    // Report Size: 1
    0x95, 0x08,    // Report Count: 8
    0x81, 0x02,    // Input: Data (2)

    // reserved
    0x95, 0x01,    // Report Count: 1
    0x75, 0x08,    // Report Size: 8
    0x81, 0x01,    // Input: Constant (1)

    // LEDs
    0x95, 0x03,    // Report Count: 3
    0x75, 0x01,    // Report Size: 1
    0x05, 0x08,    // Usage Page: LEDs
    0x19, 0x01,    // Usage Minimum: Num Lock
    0x29, 0x03,    // Usage Maximum: Scroll Lock
    0x91, 0x02,    // Output: Data (2)

    // padding x 5 bits
    0x95, 0x05,    // Report Count: 5
    0x75, 0x01,    // Report Size: 1
    0x91, 0x01,    // Output: Constant (1)

    // 6 keys
    0x95, 0x06,          // Report Count: 6
    0x75, 0x08,          // Report Size: 8
    0x15, 0x00,          // Logical Minimum: 0
    0x26, 0xFF, 0x00,    // Logical Maximum: 255
    0x05, 0x07,          // Usage Page: Keyboard/Keypad
    0x19, 0x00,          // Usage Minimum: 0
    0x2A, 0xFF, 0x00,    // Usage Maximum: 255
    0x81, 0x00,          // Input: Data (0)

    0xC0,    // End collection

    // CONSUMER CONTROL DEVICE
    0x05, 0x0c,                      // Usage Page (Consumer Devices)
    0x0b, 0x01, 0x00, 0x0c, 0x00,    // USAGE (Consumer Devices:Consumer Control)
    0xa1, 0x01,                      // Collection (Application)
    0x85, 0x02,                      //   REPORT_ID (2)
    0x19, 0x00,                      //   USAGE_MINIMUM (Unassigned)
    0x2a, 0x08, 0x01,                //   USAGE_MAXIMUM (Police Alarm)
    0x15, 0x00,                      //   LOGICAL_MINIMUM (0)
    0x26, 0x3c, 0x02,                //   LOGICAL_MAXIMUM (572)
    0x95, 0x01,                      //   REPORT_COUNT (1)
    0x75, 0x10,                      //   REPORT_SIZE (16)
    0x81, 0x00,                      //   INPUT (Data,Array,Absolute)

    0xc0    // END_COLLECTION
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
// These must line up with the string descriptor array below...

#define LANGUAGE_DESC_ID 0
#define MANUFACTURER_STRING_DESC_ID 1
#define PRODUCT_NAME_STRING_DESC_ID 2
#define SERIAL_NUMBER_STRING_DESC_ID 3
#define CONFIG_STRING_DESC_ID 4

#define NUM_STRING_DESCRIPTORS 5

//////////////////////////////////////////////////////////////////////

__code const uint8 device_desc[] = {

    sizeof(device_desc),             // bLength
    USB_DESCR_TYP_DEVICE,            // bDescriptorType
    0x10,                            // bcdUSB (1)
    0x01,                            // bcdUSB (2)
    0x00,                            // bDeviceClass
    0x00,                            // bDeviceSubClass
    0x00,                            // bDeviceProtocol
    ENDPOINT_0_SIZE,                 // bMaxPacketSize0
    USB_VID & 0xff,                  // idVendor (LSB)
    USB_VID >> 8,                    // idVendor (MSB)
    USB_PID & 0xff,                  // idProduct (LSB)
    USB_PID >> 8,                    // idProduct (MSB)
    0x00,                            // bcdDevice(1)
    0x01,                            // bcdDevice(2)
    MANUFACTURER_STRING_DESC_ID,     // iManufacturer
    PRODUCT_NAME_STRING_DESC_ID,     // iProduct
    SERIAL_NUMBER_STRING_DESC_ID,    // iSerialNumber
    0x01                             // bNumConfigurations

};

//////////////////////////////////////////////////////////////////////

#define NUM_INTERFACES 2

__code const uint8 config_desc[] = {

    // Config
    0x09,                       // bLength
    USB_DESCR_TYP_CONFIG,       // bDescriptorType
    SIZEOF_LSB(config_desc),    // wTotalLength (1)
    SIZEOF_MSB(config_desc),    // wTotalLength (2)
    NUM_INTERFACES,             // bNumInterface
    0x01,                       // bConfigurationValue
    CONFIG_STRING_DESC_ID,      // iConfiguration
    0x80,                       // bmAttributes: Bus Powered/No Remote Wakeup
    50,                         // bMaxPower 50 x 2 = 100mA

    //////////////////////////////////////////////////
    // INTERFACE 0: KEYBOARD / CONSUMER CONTROL HID DEVICE

    // Interface
    0x09,                     // bLength
    USB_DESCR_TYP_INTERF,     // bDescriptorType
    0x00,                     // bInterfaceNumber
    0x00,                     // bAlternateSetting
    0x01,                     // bNumEndpoints
    USB_DEV_CLASS_HID,        // bInterfaceClass
    0x01,                     // bInterfaceSubClass
    0x01,                     // bInterfaceProtocol: Keyboard
    CONFIG_STRING_DESC_ID,    // iInterface - HID STRING

    // HID
    0x09,                        // bLength
    USB_DESCR_TYP_HID,           // bDescriptorType: HID
    0x11,                        // bcdHID(1)
    0x01,                        // bcdHID(2)
    0x00,                        // bCountryCode
    0x01,                        // bNumDescriptors
    0x22,                        // bDescriptorType: Report
    SIZEOF_LSB(hid_rep_desc),    // wDescriptorLength (1)
    SIZEOF_MSB(hid_rep_desc),    // wDescriptorLength (2)

    // Endpoint
    0x07,                      // bLength
    USB_DESCR_TYP_ENDP,        // bDescriptorType: ENDPOINT
    0x81,                      // bEndpointAddress: IN/Endpoint1
    USB_ENDP_TYPE_INTER,       // bmAttributes: Interrupt
    USB_PACKET_SIZE & 0xff,    // wMaxPacketSize (1)
    USB_PACKET_SIZE >> 8,      // wMaxPacketSize (2)
    0x02,                      // bInterval

    //////////////////////////////////////////////////
    // INTERFACE 1: CUSTOM HID DEVICE

    // Interface
    0x09,                           // bLength
    USB_DESCR_TYP_INTERF,           // bDescriptorType
    0x01,                           // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints
    USB_DEV_CLASS_HID,              // bInterfaceClass
    0x01,                           // bInterfaceSubClass
    0x01,                           // bInterfaceProtocol: Keyboard
    PRODUCT_NAME_STRING_DESC_ID,    // iInterface - PRODUCT STRING

    // CUSTOM HID
    0x09,                           // bLength
    USB_DESCR_TYP_HID,              // bDescriptorType: HID
    0x11,                           // bcdHID(1)
    0x01,                           // bcdHID(2)
    0x00,                           // bCountryCode
    0x01,                           // bNumDescriptors
    0x22,                           // bDescriptorType: Report
    SIZEOF_LSB(custom_rep_desc),    // wDescriptorLength (1)
    SIZEOF_MSB(custom_rep_desc),    // wDescriptorLength (2)

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
    0x02,                         // bInterval
};

STATIC_ASSERT(sizeof(config_desc) < 256);

//////////////////////////////////////////////////////////////////////
// Configuration descriptors

__code const usb_descriptor_t config_descs[] = {
    DESCRIPTOR(config_desc),    // HID config
};

//////////////////////////////////////////////////////////////////////
// HID report descriptors

__code const usb_descriptor_t report_descs[] = {
    DESCRIPTOR(hid_rep_desc),       // main HID report
    DESCRIPTOR(custom_rep_desc),    // custom HID report
};

//////////////////////////////////////////////////////////////////////
// String admin - UINT16s in here...

#define LANGUAGE_DESC_STRING 0x0409
#define MANUFACTURER_STRING 'T', 'i', 'n', 'y', ' ', 'L', 'i', 't', 't', 'l', 'e', ' ', 'T', 'h', 'i', 'n', 'g', 's'
#define CONFIG_STRING 'T', 'i', 'n', 'y', ' ', 'U', 'S', 'B', ' ', 'K', 'n', 'o', 'b'

#define STR_HDR(x) (SIZEOF_LSB(x) | (USB_DESCR_TYP_STRING << 8))

__code const uint16 language_desc[] = { STR_HDR(language_desc), LANGUAGE_DESC_STRING };
__code const uint16 manufacturer_string[] = { STR_HDR(manufacturer_string), MANUFACTURER_STRING };
__code const uint16 config_string[] = { STR_HDR(config_string), CONFIG_STRING };

//////////////////////////////////////////////////////////////////////
// String descriptors

__code const usb_descriptor_t string_descs[NUM_STRING_DESCRIPTORS] = {
    DESCRIPTOR(language_desc),           // #define LANGUAGE_DESC_ID             0
    DESCRIPTOR(manufacturer_string),     // #define MANUFACTURER_STRING_DESC_ID  1
    DESCRIPTOR(product_name_string),     // #define PRODUCT_NAME_STRING_DESC_ID  2 (dynamic in xdata)
    DESCRIPTOR(serial_number_string),    // #define SERIAL_NUMBER_STRING_DESC_ID 3 (dynamic in xdata)
    DESCRIPTOR(config_string)            // #define CONFIG_STRING_DESC_ID        4
};

//////////////////////////////////////////////////////////////////////

#define NUM_CONFIG_DESCS ARRAY_COUNT(config_descs)
#define NUM_REPORT_DESCS ARRAY_COUNT(report_descs)
#define NUM_STRING_DESCS ARRAY_COUNT(string_descs)
