#pragma once

//////////////////////////////////////////////////////////////////////

typedef struct save_buffer
{
    uint8 data[16];
    uint32 index;
    uint16 crc;
} save_buffer_t;

bool load_flash(save_buffer_t *buffer);
bool save_flash(save_buffer_t *buffer);
