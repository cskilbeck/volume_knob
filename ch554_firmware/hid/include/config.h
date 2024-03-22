#pragma once

//////////////////////////////////////////////////////////////////////

#define CONFIG_VERSION 0x82

//////////////////////////////////////////////////////////////////////

#define MEDIA_KEY(x) ((x) | 0x8000)
#define NORMAL_KEY(x) (x)

#define IS_MEDIA_KEY(x) ((x & 0x8000) != 0)
#define IS_KEYBOARD_KEY(x) ((x & 0x8000) == 0)

//////////////////////////////////////////////////////////////////////

typedef enum config_flags
{
    cf_led_flash_on_cw = 0x01,
    cf_led_flash_on_ccw = 0x02,
    cf_led_flash_on_press = 0x04,
    cf_led_flash_on_release = 0x08,
    cf_reverse_rotation = 0x10,

} config_flags_t;

//////////////////////////////////////////////////////////////////////

struct config
{
    uint8 version;

    // what keys to send for rotation / press/release
    uint16 key_clockwise;
    uint16 key_counterclockwise;
    uint16 key_press;

    // if key_release != 0 then don't send key up after key_press
    // and send key_release when released

    uint8 mod_clockwise;
    uint8 mod_counterclockwise;
    uint8 mod_press;

    uint16 flags;

    uint8 pad[CONFIG_MAX_LEN - 12];
};

typedef struct config config_t;

//////////////////////////////////////////////////////////////////////

#define CONFIG_SIZE (sizeof(config_t))

_Static_assert(CONFIG_SIZE == CONFIG_MAX_LEN);

//////////////////////////////////////////////////////////////////////

bool load_config();
bool save_config();
