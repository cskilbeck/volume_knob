#include "main.h"

//////////////////////////////////////////////////////////////////////

#if DEVICE == DEVICE_DEVKIT
#define ROTATE_REVERSE cf_reverse_rotation
#else
#define ROTATE_REVERSE 0
#endif

//////////////////////////////////////////////////////////////////////

__code const config_t default_config = {
    CONFIG_VERSION,                                                                      //
    MEDIA_KEY(KEY_MEDIA_VOLUMEUP),                                                       //
    MEDIA_KEY(KEY_MEDIA_VOLUMEDOWN),                                                     //
    MEDIA_KEY(KEY_MEDIA_MUTE),                                                           //
    0,                                                                                   //
    0,                                                                                   //
    0,                                                                                   //
    0,                                                                                   //
    0,                                                                                   //
    cf_led_flash_on_cw | cf_led_flash_on_ccw | cf_led_flash_on_press | ROTATE_REVERSE    //
};

//////////////////////////////////////////////////////////////////////

bool load_config()
{
    if(!flash_load(&config, sizeof(config_t)) || config.version != CONFIG_VERSION) {
        printf("No config (or wrong version: %d)\n", config.version);
        memcpy(&config, &default_config, sizeof(config_t));
    }
    printf("Config version: %d\n", config.version);
    return true;
}

//////////////////////////////////////////////////////////////////////

bool save_config()
{
    return flash_save(&config, sizeof(config_t));
}
