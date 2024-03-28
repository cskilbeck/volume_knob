#include "main.h"

//////////////////////////////////////////////////////////////////////

#if DEVICE == DEVICE_DEVKIT
#define ROTATE_REVERSE cf_reverse_rotation
#else
#define ROTATE_REVERSE 0
#endif

//////////////////////////////////////////////////////////////////////

__code const hid_config_t default_hid_config = {
    CONFIG_VERSION,                                                     //
    MEDIA_KEY(KEY_MEDIA_VOLUMEUP),                                      //
    MEDIA_KEY(KEY_MEDIA_VOLUMEDOWN),                                    //
    MEDIA_KEY(KEY_MEDIA_MUTE),                                          //
    0,                                                                  //
    0,                                                                  //
    0,                                                                  //
    cf_led_flash_on_cw | cf_led_flash_on_ccw | cf_led_flash_on_press    //
};

//////////////////////////////////////////////////////////////////////

bool load_config()
{
    if(!flash_load(&hid_config, sizeof(hid_config_t)) || hid_config.version != CONFIG_VERSION) {
        printf("No config (or wrong version: %d)\n", hid_config.version);
        memcpy(&hid_config, &default_hid_config, sizeof(hid_config_t));
    }
    printf("Config version: %d\n", hid_config.version);
    return true;
}

//////////////////////////////////////////////////////////////////////

bool save_config()
{
    return flash_save(&hid_config, sizeof(hid_config_t));
}
