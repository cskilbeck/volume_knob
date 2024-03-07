#include <string.h>
#include "ch554.h"
#include "types.h"
#include "debug.h"
#include "util.h"
#include "config.h"

//////////////////////////////////////////////////////////////////////

static uint32 current_save_index = 1;
static uint8 current_save_offset = 0;

static uint8 const accel_values[2][4] = { { 0, 2, 10, 25 }, { 0, 20, 50, 100 } };

#define DEFAULT_FLAGS cf_led_flash_on_limit | cf_led_flash_on_click | cf_acceleration_lsb

__code const config_t default_config = {

    CONFIG_VERSION,    // uint8 version;               // config struct version - must be 1st byte!
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
    DEFAULT_FLAGS      // uint16 flags;                // flags, see enum above
};

//////////////////////////////////////////////////////////////////////
// read bytes from the data flash area

static bool read_flash_data(uint8 flash_addr, uint8 num_bytes, uint8 *data)
{
    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    flash_addr <<= 1;

    while(num_bytes != 0) {

        ROM_ADDR_L = flash_addr;
        ROM_CTRL = ROM_CMD_READ;

        if((ROM_STATUS & bROM_CMD_ERR) != 0) {
            return false;
        }

        *data++ = ROM_DATA_L;
        flash_addr += 2;
        num_bytes -= 1;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////
// write bytes to the data flash area

static bool write_flash_data(uint8 flash_addr, uint8 num_bytes, uint8 const *data)
{
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG |= bDATA_WE;

    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    flash_addr <<= 1;

    while(num_bytes != 0) {

        ROM_ADDR_L = flash_addr;
        ROM_DATA_L = *data++;
        ROM_CTRL = ROM_CMD_WRITE;

        if((ROM_STATUS & bROM_CMD_ERR) != 0) {
            return false;
        }
        flash_addr += 2;
        num_bytes -= 1;
    }

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG &= ~bDATA_WE;

    return true;
}

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
        if(read_flash_data(offset, FLASH_SLOT_SIZE, (uint8 *)&save_buffer) &&    // can load this slot
           save_buffer_check_crc(&save_buffer) &&                                // and it's valid
           save_buffer.index > highest_index) {                                  // and it's the most recently saved

            highest_index = save_buffer.index;
            offset_found = offset;
        }
        offset += FLASH_SLOT_SIZE;
    }

    // if there was a valid save slot, load it

    if(offset_found != 0xff &&                                                     // found one
       read_flash_data(offset_found, FLASH_SLOT_SIZE, (uint8 *)&save_buffer) &&    // and can load it
       save_buffer_check_crc(&save_buffer)) {                                      // and it's valid

        print_uint8("Load config from offset ", offset_found);

        // prepare for next save
        current_save_index = highest_index;
        current_save_offset = offset_found;
        memcpy(&config, save_buffer.data, sizeof(config_t));
        return true;
    }

    // no valid save was found, return default_config
    putstr("no saved config found\n");
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
    print_uint8("save config to offset", current_save_offset);
    print_uint32("save index", current_save_index);
    return write_flash_data(current_save_offset, FLASH_SLOT_SIZE, (uint8 *)&save_buffer);
}

//////////////////////////////////////////////////////////////////////

uint8 get_acceleration()
{
    uint8 accel_index = (config.flags >> 10) & 3;
    uint8 accel_table = (config.flags & cf_rotate_extended) ? 1 : 0;
    return accel_values[accel_table][accel_index];
}
