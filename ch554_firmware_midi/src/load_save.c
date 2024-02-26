#include <string.h>
#include "ch554.h"
#include "types.h"
#include "debug.h"
#include "util.h"
#include "load_save.h"

//////////////////////////////////////////////////////////////////////

static uint32 current_save_index = 1;
static uint8 current_save_slot = 0;

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
    uint16 check = save_buffer_get_crc(buffer);
    return check == buffer->crc;
}

//////////////////////////////////////////////////////////////////////
// return 0 (we loaded it from slot 0), 1 (we loaded it from slot 1) or -1 (no valid save found)

bool load_flash(save_buffer_t *buffer)
{
    bool valid[2] = { false, false };
    uint32 index[2];

    if(read_flash_data(0, sizeof(save_buffer_t), (uint8 *)buffer)) {
        valid[0] = save_buffer_check_crc(buffer);
        index[0] = buffer->index;
    }

    if(read_flash_data(sizeof(save_buffer_t), sizeof(save_buffer_t), (uint8 *)buffer)) {
        valid[1] = save_buffer_check_crc(buffer);
        index[1] = buffer->index;
    }

    if(valid[1] && (index[1] > index[0] || valid[0] == false)) {
        current_save_index = index[1];
        current_save_slot = 1;
        putstr("load slot 1\n");
        return true;
    }

    if(read_flash_data(0, sizeof(save_buffer_t), (uint8 *)buffer)) {
        valid[0] = save_buffer_check_crc(buffer);
        index[0] = buffer->index;
        if(valid[0]) {
            current_save_index = index[0];
            current_save_slot = 0;
            putstr("load slot 0\n");
            return true;
        }
    }
    putstr("no save found\n");
    return false;
}

//////////////////////////////////////////////////////////////////////

bool save_flash(save_buffer_t *buffer)
{
    current_save_slot = 1 - current_save_slot;
    current_save_index += 1;
    buffer->index = current_save_index;
    save_buffer_set_crc(buffer);
    hexdump("save to slot", &current_save_slot, 1);
    hexdump("save index", &current_save_index, 4);
    uint8 flash_addr = (current_save_slot == 0) ? 0 : sizeof(save_buffer_t);
    return write_flash_data(flash_addr, sizeof(save_buffer_t), (uint8 *)buffer);
}
