//////////////////////////////////////////////////////////////////////

#include "main.h"

static __idata uint32 current_save_index = 1;
static __idata uint8 current_save_offset = 0;

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

static uint16 crc16(uint8 const *data, uint8 length)
{
    uint16 crc = 0xffff;

    while(length != 0) {
        uint16 x = (uint16)((uint8)(crc >> 8) ^ *data++);
        x ^= x >> 4;
        crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
        length -= 1;
    }
    return crc;
}

//////////////////////////////////////////////////////////////////////

static uint16 save_buffer_get_crc(save_buffer_t const *buffer)
{
    return crc16(buffer->data, sizeof(buffer->data) + sizeof(buffer->index));
}

//////////////////////////////////////////////////////////////////////

static void save_buffer_set_crc(save_buffer_t *buffer)
{
    buffer->crc = save_buffer_get_crc(buffer);
}

//////////////////////////////////////////////////////////////////////

static bool save_buffer_check_crc(save_buffer_t const *buffer)
{
    if(save_buffer.data[0] != CONFIG_VERSION) {
        return false;
    }
    uint16 check = save_buffer_get_crc(buffer);
    return check == buffer->crc;
}

//////////////////////////////////////////////////////////////////////
// return 0 (we loaded it from slot 0), 1 (we loaded it from slot 1) or -1 (no valid save found)

bool load_config()
{
    uint32 highest_index = 0;
    uint8 offset_found = 0xff;

    // find the save slot with the highest index

    uint8 offset = 0;
    for(uint8 i = 0; i < FLASH_NUM_SLOTS; ++i) {

        read_flash_data(offset, FLASH_SLOT_SIZE, (uint8 *)&save_buffer);

        if(save_buffer_check_crc(&save_buffer) &&    // if it's valid
           save_buffer.index > highest_index) {      // and it's the most recently saved

            highest_index = save_buffer.index;
            offset_found = offset;
        }
        offset += FLASH_SLOT_SIZE;
    }

    // if there was a valid save slot, load it

    if(offset_found != 0xff) {    // found one

        read_flash_data(offset_found, FLASH_SLOT_SIZE, (uint8 *)&save_buffer);    // and can load it

        if(save_buffer_check_crc(&save_buffer)) {    // and it's valid

            printf("Load config from offset %d (%02x)\n", offset_found, offset_found);

            // prepare for next save
            current_save_index = highest_index;
            current_save_offset = offset_found;
            memcpy(&config, save_buffer.data, sizeof(config_t));
            return true;
        }
    }
    // no valid save was found, return default_config
    puts("no saved config found");
    memcpy(&config, &default_config, sizeof(config_t));
    return false;
}

//////////////////////////////////////////////////////////////////////

bool save_config()
{
    memcpy(&save_buffer.data, &config, sizeof(config_t));
    current_save_offset += FLASH_SLOT_SIZE;
    if(current_save_offset >= FLASH_SIZE) {
        current_save_offset = 0;
    }
    current_save_index += 1;
    save_buffer.index = current_save_index;
    save_buffer_set_crc(&save_buffer);
    printf("save config to offset %d (%02x)\n", current_save_offset, current_save_offset);
    printf("save index %ld\n", current_save_index);
    write_flash_data(current_save_offset, FLASH_SLOT_SIZE, (uint8 *)&save_buffer);
    return true;
}

//////////////////////////////////////////////////////////////////////

uint8 get_acceleration()
{
    uint8 accel_index = config.acceleration & 3;
    uint8 accel_table = config_flag(cf_rotate_extended) ? 1 : 0;
    return accel_values[accel_table][accel_index];
}
