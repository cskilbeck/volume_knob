//////////////////////////////////////////////////////////////////////

#include "main.h"

//////////////////////////////////////////////////////////////////////

void goto_bootloader()
{
    // flash the led a few times
    led_flash_n_times(BOOTLOADER_FLASH_LED_COUNT, BOOTLOADER_FLASH_LED_SPEED);

    // shutdown peripherals
    EA = 0;
    USB_CTRL = 0;
    UDEV_CTRL = 0;

    // and jump to bootloader
    ((void (*)())BOOT_LOAD_ADDR)();
}

//////////////////////////////////////////////////////////////////////
// read bytes from the data flash area

void read_flash_data(uint8 flash_addr, uint8 len, uint8 *data)
{
    flash_addr <<= 1;

    while(len-- != 0) {

        ROM_ADDR_L = flash_addr;
        ROM_ADDR_H = DATA_FLASH_ADDR >> 8;

        ROM_CTRL = ROM_CMD_READ;

        if((ROM_STATUS & bROM_CMD_ERR) != 0) {
            break;
        }

        *data++ = ROM_DATA_L;

        flash_addr += 2;
    }
}

//////////////////////////////////////////////////////////////////////
// write bytes to the data flash area

void write_flash_data(uint8 flash_addr, uint8 len, uint8 *data)
{
    flash_addr <<= 1;

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;

    GLOBAL_CFG |= bDATA_WE;

    while(len-- != 0) {

        ROM_ADDR_L = flash_addr;
        ROM_ADDR_H = DATA_FLASH_ADDR >> 8;

        ROM_DATA_L = *data++;

        ROM_CTRL = ROM_CMD_WRITE;

        if((ROM_STATUS & bROM_CMD_ERR) != 0) {
            break;
        }

        flash_addr += 2;
    }

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;

    GLOBAL_CFG &= ~bDATA_WE;
}

//////////////////////////////////////////////////////////////////////

void hard_fault()
{
    while(1) {
        LED_BIT ^= 1;
        for(uint16_t i = 1; i != 0; ++i) {
        }
    }
}
