#pragma once

//////////////////////////////////////////////////////////////////////
// See usb_config.h
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

enum hid_custom_command
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
};

//////////////////////////////////////////////////////////////////////

enum hid_custom_response
{
    hcc_here_is_config = 1,
    hcc_here_is_firmware_version = 2,
    hcc_led_flashed = 3,
    hcc_set_config_ack = 4
};
