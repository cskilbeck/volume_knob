#include "main.h"

//////////////////////////////////////////////////////////////////////

static uint32 current_save_index = 1;
static uint8 current_save_offset = 0;

//////////////////////////////////////////////////////////////////////

static uint16 crc16(uint8 const *src_data, uint8 length)
{
    uint16 crc = 0xffff;

    while(length != 0) {
        uint16 x = (uint16)((uint8)(crc >> 8) ^ *src_data++);
        x ^= x >> 4;
        crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
        length -= 1;
    }
    return crc;
}

//////////////////////////////////////////////////////////////////////

static uint16 save_buffer_get_crc(save_buffer_t const *buffer)
{
    return crc16(buffer->bytes, sizeof(buffer->bytes) + sizeof(buffer->index));
}

//////////////////////////////////////////////////////////////////////

static void save_buffer_set_crc(save_buffer_t *buffer)
{
    buffer->crc = save_buffer_get_crc(buffer);
}

//////////////////////////////////////////////////////////////////////

static bool save_buffer_check_crc(save_buffer_t const *buffer)
{
    if(save_buffer.bytes[0] != CONFIG_VERSION) {
        return false;
    }
    uint16 check = save_buffer_get_crc(buffer);
    return check == buffer->crc;
}

//////////////////////////////////////////////////////////////////////
// return 0 (we loaded it from slot 0), 1 (we loaded it from slot 1) or -1 (no valid save found)

bool flash_load(void __xdata *config, size_t config_size)
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

            printf("Load config from offset %d (0x%x)\n", offset_found, offset_found);

            // prepare for next save
            current_save_index = highest_index;
            current_save_offset = offset_found;
            memcpy(config, save_buffer.bytes, config_size);
            return true;
        }
    }
    // no valid save was found, return default_config
    puts("no saved config found");
    // memcpy(&config, &default_config, sizeof(config_t));
    return false;
}

//////////////////////////////////////////////////////////////////////

bool flash_save(void __xdata *config, size_t config_size)
{
    memcpy(&save_buffer.bytes, config, config_size);
    current_save_offset += FLASH_SLOT_SIZE;
    if(current_save_offset >= FLASH_SIZE) {
        current_save_offset = 0;
    }
    current_save_index += 1;
    save_buffer.index = current_save_index;
    save_buffer_set_crc(&save_buffer);
    printf("save config to offset %d (0x%x)\n", current_save_offset, current_save_offset);
    printf("save index %ld\n", current_save_index);
    write_flash_data(current_save_offset, FLASH_SLOT_SIZE, (uint8 *)&save_buffer);
    return true;
}

//////////////////////////////////////////////////////////////////////
