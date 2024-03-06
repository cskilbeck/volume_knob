#pragma once

// max USB transfer size is 64 bytes in high speed mode
// size of a midi_packet = 4 bytes
// each midi_packet contains 3 x 7 bit values
// so we get max_usb_size / sizeof(midi_packet) = 64 / 4 = 48 x 7 bit values per transfer
// and (48 x 7) / 8 = 42 bytes per transfer
// save_footer needs 6 bytes (2 for CRC, 4 for save_index)
// so absolute max size of config is 42 - 6 = 36 bytes
// BUT: in order to fit 4 save slots in the 128 byte flash (for wear levelling)
// set the max slot size to 32.
// so max config size = max_slot_size - sizeof(save_footer) = 32 - 6 = 26

// also, for simplicity - don't support transfers which are split across multiple usb packets
//  which rules out low speed usb where max transfer size is 8 bytes

#define FLASH_SIZE 128
#define FLASH_SLOT_SIZE 32
#define FLASH_NUM_SLOTS (FLASH_SIZE / FLASH_SLOT_SIZE)

#define CONFIG_MAX_LEN (FLASH_SLOT_SIZE - 6)

//////////////////////////////////////////////////////////////////////
// increment this whenever the format of the config changes so the web ui knows to ignore old ones

#define CONFIG_VERSION 0x06

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

    // current button toggle state
    cf_toggle = 0x1000
};

//////////////////////////////////////////////////////////////////////
// sizeof config_t must be CONFIG_LEN

typedef struct config
{
    // config struct version - must be 1st byte!
    uint8 version;

    // Control Change index MSB,LSB for knob
    uint8 rot_control[2];

    // Control Change index MSB,LSB for button
    uint8 btn_control[2];

    // 1st,2nd button values or pressed/released values if cf_btn_momentary
    uint16 btn_value[2];

    // rotate channel in low nibble, button in high nibble
    uint8 channels;

    // Zero point in relative mode
    uint16 rot_zero_point;

    // How much to change by
    uint16 rot_delta;

    // lower limit of value in absolute mode
    uint16 rot_limit_low;

    // upper limit of value in absolute mode
    uint16 rot_limit_high;

    // current value (in absolute mode)
    uint16 rot_current_value;

    // flags, see enum above
    uint16 flags;

} config_t;

_Static_assert(sizeof(config_t) < CONFIG_MAX_LEN);

extern __code const config_t default_config;
extern __xdata config_t config;

inline uint8 get_rot_channel()
{
    return config.channels & 0xf;
}

inline uint8 get_btn_channel()
{
    return config.channels >> 4;
}

inline bool is_toggle_mode()
{
    return (config.flags & cf_btn_momentary) == 0;
}

inline bool get_acceleration()
{
    return (config.flags >> 10) & 3;
}

//////////////////////////////////////////////////////////////////////

typedef struct save_buffer
{
    uint8 data[CONFIG_MAX_LEN];
    uint32 index;
    uint16 crc;
} save_buffer_t;

_Static_assert(sizeof(save_buffer_t) == 32);

extern __xdata save_buffer_t save_buffer;

bool load_config();
bool save_config();
