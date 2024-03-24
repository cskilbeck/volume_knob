#pragma once

//////////////////////////////////////////////////////////////////////
// VID = 16D0, PID = 1317

#define USB_VID 0x16D0
#define USB_PID 0x1317

//////////////////////////////////////////////////////////////////////
// Device descriptor

__code uint8 const device_desc[] = {
    sizeof(device_desc),     // length
    USB_DESCR_TYP_DEVICE,    // USB_DESC_TYPE_DEVICE
    0x10,                    // USB v2.01 LOW BCD
    0x01,                    // USB v2.01 HIGH BCD
    0x00,                    // device class
    0x00,                    // device subclass
    0x00,                    // device protocol
    DEFAULT_ENDP0_SIZE,      // Define it in interface level
    USB_VID & 0xff,          // USB_VID (low)
    USB_VID >> 8,            // USB_VID (high)
    USB_PID & 0xff,          // USB_PID (low)
    USB_PID >> 8,            // USB_PID (high)
    0x00,                    // device rel LOW BCD
    0x01,                    // device rel HIGH BCD
    0x01,                    // manufacturer string id
    0x02,                    // product string id
    0x03,                    // serial number string id
    0x01                     // num configurations
};

//////////////////////////////////////////////////////////////////////
// Config descriptor

__code uint8 const midi_config_desc[] = {

    // Configuration descriptor (two interfaces)
    0x09,                               // length
    USB_DESCR_TYP_CONFIG,               // bDescriptorType
    sizeof(midi_config_desc) & 0xff,    // wTotalLength LSB
    sizeof(midi_config_desc) >> 8,      // wTotalLength MSB
    0x02,                               // bNumInterface
    0x01,                               // bConfigurationValue
    0x00,                               // iConfiguration string descriptor index
    0x80,                               // bmAttributes Bus Powered, No remote wake
    50,                                 // bMaxPower, 50 x 2 = 100 mA

    // Interface 0 USB Audio Class
    0x09,                    // bLength
    USB_DESCR_TYP_INTERF,    // bDescriptorType
    0x00,                    // bInterfaceNumber
    0x00,                    // bAlternateSetting
    0x00,                    // bNumEndpoints
    USB_DEV_CLASS_AUDIO,     // bInterfaceClass
    0x01,                    // bInterfaceSubClass (Audio Device)
    0x00,                    // bInterfaceProtocol
    0x00,                    // iInterface string descriptor index

    // Interface 0 function descriptor
    0x09,                     // bLength
    USB_DESCR_TYP_CS_INTF,    // Class interface
    0x01,                     // Class interface: Header
    0x00,                     // Audio Device Class 1.0 LSB
    0x01,                     // Audio Device Class 1.0 MSB
    0x09,                     // wTotalLength LSB
    0x00,                     // wTotalLength MSB
    0x01,                     // bInCollection 1
    0x01,                     // bInterfaceNumber 1

    // Interface 1 MIDI Streaming Interface
    0x09,                    // bLength
    USB_DESCR_TYP_INTERF,    // bDescriptorType
    0x01,                    // bInterfaceNumber
    0x00,                    // bAlternateSetting
    0x02,                    // bNumEndpoints
    USB_DEV_CLASS_AUDIO,     // bInterfaceClass
    0x03,                    // bInterfaceSubClass: MIDI Streaming
    0x00,                    // bInterfaceProtocol
    0x00,                    // iInterface string descriptor index

    // Interface 1 function descriptor
    0x07,                     // bLength
    USB_DESCR_TYP_CS_INTF,    // Class interface
    0x01,                     // Class interface: Header
    0x00,                     // Midi Streaming v1.0 LSB
    0x01,                     // Midi Streaming v1.0 MSB
    22,                       // wTotalLength LSB
    0x00,                     // wTotalLength MSB

    // IN-JACK
    0x06,                     // bLength
    USB_DESCR_TYP_CS_INTF,    // CS_INTERFACE
    0x02,                     // MIDI_IN_JACK
    0x01,                     // embedded
    0x01,                     // jack ID
    0x00,                     // iJack

    // OUT-JACK
    0x09,                     // bLength
    USB_DESCR_TYP_CS_INTF,    // CS_INTERFACE
    0x03,                     // MIDI_OUT_JACK
    0x01,                     // embedded
    0x02,                     // jack ID
    0x00,                     // input pins
    0x00,                     // source ID
    0x00,                     // source pin
    0x00,                     // iJack

    //////////

    // then 2 endpoints
    0x07,                      // bLength
    USB_DESCR_TYP_ENDP,        // ENDPOINT
    0x02,                      // IN, EP2
    0x02,                      // bulk, not shared
    USB_PACKET_SIZE & 0xff,    // max packet size LSB
    USB_PACKET_SIZE >> 8,      // max packet size MSB
    0x00,                      // bInterval (unused for bulk EP)

    0x05,                     // bLength
    USB_DESCR_TYP_CS_ENDP,    // class specific endpoint
    0x01,                     // subtype 1
    0x01,                     // bNumberOfJacks
    0x01,                     // Jack ID 1

    0x07,                      // bLength
    USB_DESCR_TYP_ENDP,        // ENDPOINT
    0x82,                      // OUT, EP2
    0x02,                      // bulk, not shared
    USB_PACKET_SIZE & 0xff,    // max packet size LSB
    USB_PACKET_SIZE >> 8,      // max packet size MSB
    0x00,                      // bInterval (unused for bulk EP)

    0x05,                     // bLength
    USB_DESCR_TYP_CS_ENDP,    // class specific endpoint
    0x01,                     // subtype 1
    0x01,                     // bNumberOfJacks
    0x02                      // Jack ID 2

};

//////////////////////////////////////////////////////////////////////
// UINT16s in here...

#define LANGUAGE_DESC 0x0409
#define MANUFACTURER_STRING 'T', 'i', 'n', 'y', ' ', 'L', 'i', 't', 't', 'l', 'e', ' ', 'T', 'h', 'i', 'n', 'g', 's'
#define AUDIO_STRING 'A', 'u', 'd', 'i', 'o'
#define JACK_STRING 'J', 'a', 'c', 'k'

#define STR_HDR(x) (sizeof(x) | (USB_DESCR_TYP_STRING << 8))

__code const uint16 language_desc[] = { STR_HDR(language_desc), LANGUAGE_DESC };
__code const uint16 manufacturer_string[] = { STR_HDR(manufacturer_string), MANUFACTURER_STRING };
__code const uint16 audio_string[] = { STR_HDR(audio_string), AUDIO_STRING };
__code const uint16 jack_string[] = { STR_HDR(jack_string), JACK_STRING };

//////////////////////////////////////////////////////////////////////

__code const usb_descriptor_t config_descs[] = {
    DESCRIPTOR(midi_config_desc)    // USB config
};

//////////////////////////////////////////////////////////////////////

__code const usb_descriptor_t string_descs[] = {
    DESCRIPTOR(language_desc),           // 0
    DESCRIPTOR(manufacturer_string),     // 1
    DESCRIPTOR(product_name_string),     // 2 - dynamic in xdata
    DESCRIPTOR(serial_number_string),    // 3 - dynamic in xdata

    DESCRIPTOR(audio_string),    // 4
    DESCRIPTOR(jack_string)      // 5
};

//////////////////////////////////////////////////////////////////////
// undef NUM_XXX_DESCS if there aren't any of that type

#define NUM_CONFIG_DESCS ARRAY_COUNT(config_descs)
#undef NUM_REPORT_DESCS
#define NUM_STRING_DESCS ARRAY_COUNT(string_descs)
