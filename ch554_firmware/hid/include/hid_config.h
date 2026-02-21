#pragma once

//////////////////////////////////////////////////////////////////////

#define CONFIG_VERSION 0x82

//////////////////////////////////////////////////////////////////////

// Type tags (bits 14:12 when bit15=0)
#define KEY_TYPE_KEYBOARD   0x0000
#define KEY_TYPE_MBTN       0x1000
#define KEY_TYPE_MWHEEL_V   0x2000
#define KEY_TYPE_MWHEEL_H   0x3000
#define KEY_TYPE_MMOVE_X    0x4000
#define KEY_TYPE_MMOVE_Y    0x5000

// Type checks
#define IS_CONSUMER_KEY(x)   ((uint16)(x) & 0x8000)
#define KEY_TYPE(x)          ((uint16)(x) & 0x7000)
#define IS_KEYBOARD_KEY(x)   (!IS_CONSUMER_KEY(x) && KEY_TYPE(x) == KEY_TYPE_KEYBOARD)
#define IS_MOUSE_BUTTON(x)   (!IS_CONSUMER_KEY(x) && KEY_TYPE(x) == KEY_TYPE_MBTN)
#define IS_MOUSE_WHEEL_V(x)  (!IS_CONSUMER_KEY(x) && KEY_TYPE(x) == KEY_TYPE_MWHEEL_V)
#define IS_MOUSE_WHEEL_H(x)  (!IS_CONSUMER_KEY(x) && KEY_TYPE(x) == KEY_TYPE_MWHEEL_H)
#define IS_MOUSE_MOVE_X(x)   (!IS_CONSUMER_KEY(x) && KEY_TYPE(x) == KEY_TYPE_MMOVE_X)
#define IS_MOUSE_MOVE_Y(x)   (!IS_CONSUMER_KEY(x) && KEY_TYPE(x) == KEY_TYPE_MMOVE_Y)

// Value extraction
#define KEY_VALUE_U8(x)      ((uint8)((x) & 0x00FF))
#define KEY_VALUE_I8(x)      ((int8)((x) & 0x00FF))

// Constructors
#define NORMAL_KEY(x)        (x)
#define CONSUMER_KEY(x)      ((x) | 0x8000)
#define MEDIA_KEY(x)         CONSUMER_KEY(x)
#define MOUSE_BUTTON(x)      (KEY_TYPE_MBTN     | ((uint8)(x)))
#define MOUSE_WHEEL_V(x)     (KEY_TYPE_MWHEEL_V | ((uint8)(int8)(x)))
#define MOUSE_WHEEL_H(x)     (KEY_TYPE_MWHEEL_H | ((uint8)(int8)(x)))
#define MOUSE_MOVE_X(x)      (KEY_TYPE_MMOVE_X  | ((uint8)(int8)(x)))
#define MOUSE_MOVE_Y(x)      (KEY_TYPE_MMOVE_Y  | ((uint8)(int8)(x)))

//////////////////////////////////////////////////////////////////////

typedef enum hid_config_flags
{
    cf_led_flash_on_cw = 0x01,
    cf_led_flash_on_ccw = 0x02,
    cf_led_flash_on_press = 0x04,
    cf_led_flash_on_release = 0x08,
    cf_reverse_rotation = 0x10,
    cf_mousewheel_DEPRECATED = 0x20,

} hid_config_flags_t;

//////////////////////////////////////////////////////////////////////

struct hid_config
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

typedef struct hid_config hid_config_t;

//////////////////////////////////////////////////////////////////////

#define CONFIG_SIZE (sizeof(hid_config_t))

STATIC_ASSERT(CONFIG_SIZE == CONFIG_MAX_LEN);

//////////////////////////////////////////////////////////////////////

bool load_config();
bool save_config();
