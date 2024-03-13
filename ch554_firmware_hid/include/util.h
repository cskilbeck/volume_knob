#pragma once

volatile const __code __at(ROM_CHIP_ID_LO)
uint16_t CHIP_UNIQUE_ID_LO;

volatile const __code __at(ROM_CHIP_ID_HI)
uint16_t CHIP_UNIQUE_ID_HI;

extern uint32 chip_id;
extern uint32 chip_id_28;

void init_chip_id();

void flash_led(uint8 n, uint8 speed);
void goto_bootloader();

void write_flash_data(uint8 flash_addr, uint8 len, uint8 *data);
void read_flash_data(uint8 flash_addr, uint8 len, uint8 *data);

void hard_fault();