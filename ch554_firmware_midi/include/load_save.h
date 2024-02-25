#pragma once

// # of bytes to load/save not including index/crc
// cannot be more than... ooh, I dunno, let's say around 26

#define FLASH_LEN 26

//////////////////////////////////////////////////////////////////////
// sizeof config_t must be FLASH_LEN

typedef struct config
{
    uint8 data[FLASH_LEN];

} config_t;

//////////////////////////////////////////////////////////////////////

typedef struct save_buffer
{
    uint8 data[sizeof(config_t)];
    uint32 index;
    uint16 crc;
} save_buffer_t;

bool load_flash(save_buffer_t *buffer);
bool save_flash(save_buffer_t *buffer);
