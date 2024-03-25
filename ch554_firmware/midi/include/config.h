#pragma once

//////////////////////////////////////////////////////////////////////
// increment this whenever the format of the config changes so the web ui knows to ignore old ones
//
// KEEP THIS IN SYNC WITH THE WEB UI, THAT'S A HASSLE!!!!

#define CONFIG_VERSION 0x0A

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

    // current button toggle state
    cf_toggle = 0x0400,

    // button's second value ('released') tracks rotation value (for e.g. mute/unmute)
    cf_button_tracks_rotation = 0x0800,

    // support both kinds of encoder
    cf_rotate_reverse = 0x1000
};

//////////////////////////////////////////////////////////////////////
// sizeof config_t must be CONFIG_MAX_LEN

struct config
{
    uint8 config_version;           // Config struct version - must be 1st byte!
    uint8 rot_control_msb;          // CC index MSB for knob
    uint8 rot_control_lsb;          // CC index LSB for knob
    uint8 btn_control_msb;          // CC index MSB for button
    uint8 btn_control_lsb;          // CC index LSB for button
    uint16 btn_value_a_14;          // 1st button value (or pressed value if cf_btn_momentary) (14 bit mode)
    uint16 btn_value_b_14;          // 2nd button value (or released value if cf_btn_momentary) (14 bit mode)
    uint8 btn_value_a_7;            // 1st button value (or pressed value if cf_btn_momentary) (7 bit mode)
    uint8 btn_value_b_7;            // 2nd button value (or released value if cf_btn_momentary) (7 bit mode)
    uint8 channels;                 // rotate channel in low nibble, button channel in high nibble
    uint8 rot_zero_point;           // Zero point in relative mode (relative mode forces 7 bit mode)
    uint16 rot_delta_14;            // Rotate delta (14 bit mode)
    uint8 rot_delta_7;              // Rotate delta (7 bit mode)
    uint16 rot_current_value_14;    // Current value (in absolute mode) (14 bit mode)
    uint8 rot_current_value_7;      // Current value (in absolute mode) (7 bit mode)
    uint8 acceleration;             // 0..3 see table in config.c
    uint16 flags;                   // Flags, see enum above

    uint8 pad[CONFIG_MAX_LEN - 22];
};

typedef struct config config_t;

#define CONFIG_SIZE (sizeof(config_t))

STATIC_ASSERT(CONFIG_SIZE == CONFIG_MAX_LEN);

extern __code const config_t default_config;

inline uint8 get_rot_channel()
{
    return config.channels & 0xf;
}

inline uint8 get_btn_channel()
{
    return config.channels >> 4;
}

inline bool config_flag(uint16 mask)
{
    return (config.flags & mask) != 0;
}

//////////////////////////////////////////////////////////////////////

uint8 get_acceleration();
bool load_config();
bool save_config();
