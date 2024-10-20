#pragma once

//////////////////////////////////////////////////////////////////////

#define HID_QUEUE_LEN 16

typedef STRUCT_QUEUE(uint8, HID_QUEUE_LEN) hid_queue_t;

//////////////////////////////////////////////////////////////////////

#define KEY_MEDIA_MUTE 0xe2
#define KEY_MEDIA_VOLUMEUP 0xe9
#define KEY_MEDIA_VOLUMEDOWN 0xea

//////////////////////////////////////////////////////////////////////
// See usb_config.c
// these structs must match the hid report descriptor layouts exactly

typedef struct _consumer_control_hid_report
{
    uint8 report_id;
    uint16 keycode;

} consumer_control_hid_report;

//////////////////////////////////////////////////////////////////////

typedef struct _keyboard_hid_report
{
    uint8 report_id;
    uint8 modifiers;
    uint8 pad;
    uint8 key[6];

} keyboard_hid_report;

//////////////////////////////////////////////////////////////////////

typedef struct _mouse_hid_report
{
    uint8 report_id;
    uint8 buttons;
    int8 x;
    int8 y;
    int8 wheel1;
    int8 wheel2;

} mouse_hid_report;

//////////////////////////////////////////////////////////////////////

typedef enum _hid_custom_command
{
    // send the config to the web ui
    hcc_get_config = 1,

    // send the firmware version to the web ui
    hcc_get_firmware_version = 2,

    // flash the led please
    hcc_flash_led = 3,

    // here's a new config
    hcc_set_config = 4,

    // reboot into firmware flashing mode
    hcc_goto_bootloader = 5

} hid_custom_command;

//////////////////////////////////////////////////////////////////////

typedef enum _hid_custom_response
{
    hcc_here_is_config = 1,
    hcc_here_is_firmware_version = 2,
    hcc_led_flashed = 3,
    hcc_set_config_ack = 4

} hid_custom_response;
