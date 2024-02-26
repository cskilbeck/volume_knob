#include <string.h>
#include <ch554.h>
#include "types.h"
#include "debug.h"
#include "util.h"

#pragma disable_warning 110

//////////////////////////////////////////////////////////////////////
// expand some bytes into an array of 7 bit values

void bytes_to_bits7(uint8 *src_data, uint8 offset, uint8 len, uint8 *dest)
{
    uint8 bits_available = 0;
    uint16 cur_src = 0;
    uint8 bits_remaining = len * 8;
    uint8 dest_index = 0;

    while(true) {

        if(bits_available < 7) {
            cur_src = (cur_src << 8) | src_data[offset++];
            bits_available += 8;
        }

        while(bits_available >= 7) {
            bits_available -= 7;
            dest[dest_index++] = (cur_src >> bits_available) & 0x7f;
            bits_remaining -= 7;
        }

        if(bits_remaining < 7) {
            dest[dest_index++] = (cur_src << (7 - bits_remaining)) & 0x7f;
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// convert some 7 bit values to bytes

void bits7_to_bytes(uint8 *src_data, uint8 offset, uint8 len, uint8 *dest)
{
    uint8 bits_available = 0;
    uint16 cur_src = 0;
    uint8 dest_index = 0;

    while(dest_index < len) {

        if(bits_available < 8) {
            cur_src = (cur_src << 7) | src_data[offset++];
            bits_available += 7;
        }

        while(bits_available >= 8) {
            bits_available -= 8;
            dest[dest_index++] = (cur_src >> bits_available) & 0xff;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// read bytes from the data flash area

bool read_flash_data(uint8 flash_addr, uint8 num_bytes, uint8 *data)
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

bool write_flash_data(uint8 flash_addr, uint8 num_bytes, uint8 const *data)
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

uint16 crc16(uint8 const *data, uint8 length)
{
    uint16 crc = 0xffff;

    while(length != 0) {
        uint16 x = (crc >> 8) ^ *data++;
        x ^= x >> 4;
        crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
        length -= 1;
    }
    return crc;
}
