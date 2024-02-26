#pragma once

void bytes_to_bits7(uint8 *src_data, uint8 offset, uint8 len, uint8 *dest);
void bits7_to_bytes(uint8 *src_data, uint8 offset, uint8 len, uint8 *dest);

volatile const __code __at(ROM_CHIP_ID_LO)
uint16_t CHIP_UNIQUE_ID_LO;

volatile const __code __at(ROM_CHIP_ID_HI)
uint16_t CHIP_UNIQUE_ID_HI;

extern uint32 chip_id;
extern uint32 chip_id_28;

void init_chip_id();
