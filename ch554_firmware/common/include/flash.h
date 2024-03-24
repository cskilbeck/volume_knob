#pragma once

// max USB transfer size is 64 bytes in high speed mode
// size of a midi_packet = 4 bytes
// each midi_packet contains 3 x 7 bit values
// so we get max_usb_size / sizeof(midi_packet) = 64 / 4 = 48 x 7 bit values per transfer
// but we lose 6 values to the sysex header and terminator so we get 42 7 bit values = 294 bits of useful data in a single USB packet
// 294 / 8 = 36 bytes (well, 288 bits = 36 bytes but we'll waste 6 bits)
// so absolute max size of config is 36 bytes
// BUT
// flash save overhead is 6 bytes (2 for CRC, 4 for save_index)
// so in fact it's 30...
// BUT
// In order to fit 4 save slots in the 128 byte flash (for wear levelling)
// set the max slot size to 32.
// so max config size = max_slot_size - sizeof(save_footer) = 32 - 6 = 26

// also, for simplicity - don't support transfers which are split across multiple usb packets
//  which rules out low speed usb where max transfer size is 8 bytes

#define FLASH_SIZE 128
#define FLASH_SLOT_SIZE 32
#define FLASH_NUM_SLOTS (FLASH_SIZE / FLASH_SLOT_SIZE)

#define CONFIG_MAX_LEN (FLASH_SLOT_SIZE - 6)

//////////////////////////////////////////////////////////////////////

typedef struct save_buffer
{
    uint8 bytes[CONFIG_MAX_LEN];
    uint32 index;
    uint16 crc;
} save_buffer_t;

STATIC_ASSERT(sizeof(save_buffer_t) == FLASH_SLOT_SIZE, "bad save_buffer size");

bool flash_load(void __xdata *config, size_t config_size);
bool flash_save(void __xdata *config, size_t config_size);
