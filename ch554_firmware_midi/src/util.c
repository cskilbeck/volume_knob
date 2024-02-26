#include <string.h>
#include <ch554.h>
#include "types.h"
#include "debug.h"
#include "util.h"

#pragma disable_warning 110

uint32 chip_id;
uint32 chip_id_28;

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

void init_chip_id()
{
    // get unique chip id
    chip_id = CHIP_UNIQUE_ID_LO | ((uint32)CHIP_UNIQUE_ID_HI << 16);

    // make a 28 bit version so we can send as 4 x 7bits for midi identification
    uint8 *cip = (uint8 *)&chip_id_28;
    cip[0] = (chip_id >> 21) & 0x7f;
    cip[1] = (chip_id >> 14) & 0x7f;
    cip[2] = (chip_id >> 7) & 0x7f;
    cip[3] = (chip_id >> 0) & 0x7f;
}
