#pragma once

void bytes_to_bits7(uint8 *src_data, uint8 offset, uint8 len, uint8 *dest);
void bits7_to_bytes(uint8 *src_data, uint8 offset, uint8 len, uint8 *dest);

bool write_flash_data(uint8 flash_addr, uint8 num_bytes, uint8 const *data);
bool read_flash_data(uint8 flash_addr, uint8 num_bytes, uint8 *data);
