//////////////////////////////////////////////////////////////////////

#include "main.h"

// should these be in here or in main.c?
static __code uint8 const accel_values[2][4] = { { 0, 1, 2, 3 }, { 0, 20, 40, 80 } };

#define DEFAULT_FLAGS cf_led_flash_on_limit | cf_led_flash_on_click

__code const config_t default_config = {

    CONFIG_VERSION,    // uint8 config_version;        // config struct version - must be 1st byte!
    7,                 // uint8 rot_control_msb;       // Control Change index MSB,LSB for knob
    39,                // uint8 rot_control_lsb;       // Control Change index MSB,LSB for knob
    3,                 // uint8 btn_control_msb;       // Control Change index MSB,LSB for button
    35,                // uint8 btn_control_lsb;       // Control Change index MSB,LSB for button
    0x3fff,            // uint16 btn_value_a_14;       // 1st,2nd button values or pressed/released values if cf_btn_momentary (14 bit mode)
    0x0000,            // uint16 btn_value_b_14;       // 1st,2nd button values or pressed/released values if cf_btn_momentary (14 bit mode)
    0x7f,              // uint8 btn_value_a_7;         // 1st,2nd button values or pressed/released values if cf_btn_momentary (7 bit mode)
    0x00,              // uint8 btn_value_b_7;         // 1st,2nd button values or pressed/released values if cf_btn_momentary (7 bit mode)
    0x00,              // uint8 channels;              // rotate channel in low nibble, button in high nibble
    0x40,              // uint8 rot_zero_point;        // Zero point in relative mode
    0x0001,            // uint16 rot_delta_14;         // How much to change by
    0x01,              // uint8 rot_delta_7;           // How much to change by
    0x0000,            // uint16 rot_current_value_14; // current value (in absolute mode) (14 bit mode)
    0x00,              // uint8 rot_current_value_7;   // current value (in absolute mode) (7 bit mode)
    0x00,              // uint8 acceleration;          // 0..3 see table in config.c
    DEFAULT_FLAGS      // uint16 flags;                // flags, see enum above
};

//////////////////////////////////////////////////////////////////////

uint8 get_acceleration()
{
    uint8 accel_index = config.acceleration & 3;
    uint8 accel_table = config_flag(cf_rotate_extended) ? 1 : 0;
    return accel_values[accel_table][accel_index];
}

//////////////////////////////////////////////////////////////////////

bool load_config()
{
    if(!flash_load(&config, sizeof(config_t))) {
        memcpy(&config, &default_config, sizeof(config));
    }
    return true;
}

//////////////////////////////////////////////////////////////////////

bool save_config()
{
    return flash_save(&config, sizeof(config_t));
}
