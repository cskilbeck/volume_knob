#pragma once

// max USB transfer size is 64 bytes
// which gives us 48 x 7 bit values (because midi is in 4 byte chunks each containing 3 x 7 bit values)
// which gives us 42 bytes per USB transfer
// so, for simplicity, define max size of config as 42 - 6 (for save index (4) and crc (2)) = 36

// simplicity = not dealing with transfers which are split across multiple usb packets,
// which rules out low speed usb where max transfer size is 8 bytes

#define FLASH_LEN 26

#define FLASH_7BIT_LEN (((FLASH_LEN * 8) + 6) / 7)

//////////////////////////////////////////////////////////////////////
// flags in config.cf_flags

typedef enum config_flags
{
    // rotate modifies both CC MSB and LSB
    cf_rotate_extended = 0x0001,

    // rotate sends relative changes
    cf_rotate_relative = 0x0002,

    // led on means off and vice versa
    cf_led_invert = 0x0004,

    // flash when knob is rotated
    cf_led_flash_on_rot = 0x0008,

    // flash when rotation limit is hit (if not cf_rotate_relative)
    cf_led_flash_on_limit = 0x0010,

    // btn sets value1/value2 based on state of button rather than toggling between them
    cf_btn_momentary = 0x0020,

    // button modifies both CC MSB and LSB
    cf_btn_extended = 0x0040,

    // flash led when button is clicked
    cf_led_flash_on_click = 0x0080,

    // flash led when button is released
    cf_led_flash_on_release = 0x0100,

    // led tracks state of button
    cf_led_track_button_toggle = 0x0200,

    // two bits for acceleration (so 4 options: off, low, med, high)
    cf_acceleration_lsb = 0x0400,
    cf_acceleration_msb = 0x0800,
};

//////////////////////////////////////////////////////////////////////
// sizeof config_t must be FLASH_LEN

typedef struct config
{
    // Control Change index MSB for knob
    uint8 rot_control_high;

    // Control Change index LSB for knob - used if cf_rotate_extended
    uint8 rot_control_low;

    union
    {
        // if cf_rotate_relative
        struct
        {
            // Zero point
            uint16 rot_zero_point;

            // How much to change by
            uint16 rot_delta;
        };

        // else if !cf_rotate_relative
        struct
        {
            // lower limit of value
            uint16 rot_limit_low;

            // upper limit of value in absolute mode
            uint16 rot_limit_high;
        };
    };

    // current value (unused if cf_rotate_relative)
    uint16 rot_current_value;

    // Control Change index MSB for button
    uint8 btn_cc_high;

    // Control Change index LSB for button - use if cf_btn_extended
    uint8 btn_cc_low;

    // 1st button toggle value or released value if cf_btn_momentary
    uint16 btn_value_1;

    // 2nd button toggle value or pressed value if cf_btn_momentary
    uint16 btn_value_2;

    // flags, see enum above
    uint16 cf_flags;

    uint8 pad[FLASH_LEN - 16];

} config_t;

//////////////////////////////////////////////////////////////////////

typedef struct save_buffer
{
    uint8 data[sizeof(config_t)];
    uint32 index;
    uint16 crc;
} save_buffer_t;

bool load_config(save_buffer_t *buffer);
bool save_config(save_buffer_t *buffer);
