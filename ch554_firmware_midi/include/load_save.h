#pragma once

// max USB transfer size is 64 bytes
// which gives us 48 x 7 bit values (because midi is in 4 byte chunks each containing 3 x 7 bit values)
// which gives us 42 bytes per USB transfer
// so, for simplicity, define max size of config as 42 - 6 (for save index (4) and crc (2)) = 36

// simplicity = not dealing with transfers which are split across multiple usb packets,
// which rules out low speed usb where max transfer size is 8 bytes

#define FLASH_LEN 26

//////////////////////////////////////////////////////////////////////
// sizeof config_t must be FLASH_LEN

typedef enum config_flags
{
    cf_rot_range14 = 1,
    cf_relative = 2,
    cf_acceleration = 4,
    cf_led_flash_on_rot = 8,
    cf_led_flash_on_limit = 16,
    cf_btn_momentary = 32,
    cf_led_flash_on_click = 64,
    cf_led_flash_on_release = 128,
    cf_led_track_button_state = 256,
    cf_led_invert = 512
};

typedef struct config
{
    uint8 rot_cc_low;
    uint8 rot_cc_high;
    uint8 rot_zero_point;
    uint8 btn_cc_low;
    uint8 btn_cc_high;
    uint8 btn_pressed_value;
    uint8 btn_released_value;
    uint16 cf_flags;

    uint8 pad[FLASH_LEN - 9];

} config_t;

//////////////////////////////////////////////////////////////////////

typedef struct save_buffer
{
    uint8 data[sizeof(config_t)];
    uint32 index;
    uint16 crc;
} save_buffer_t;

bool load_flash(save_buffer_t *buffer);
bool save_flash(save_buffer_t *buffer);
