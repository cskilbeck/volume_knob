//////////////////////////////////////////////////////////////////////

#include "main.h"

#define BOOTLOADER_FLASH_LED_COUNT 30
#define BOOTLOADER_FLASH_LED_SPEED 40

#define SOFTWARE_RESET_FLASH_LED_COUNT 30
#define SOFTWARE_RESET_FLASH_LED_SPEED 40

//////////////////////////////////////////////////////////////////////
// shutdown peripherals

static void shutdown_peripherals()
{
    EA = 0;
    USB_CTRL = 0;
    UDEV_CTRL = 0;
}

//////////////////////////////////////////////////////////////////////

void software_reset()
{
    led_flash_n_times(SOFTWARE_RESET_FLASH_LED_COUNT, SOFTWARE_RESET_FLASH_LED_SPEED);

    shutdown_peripherals();

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;

    GLOBAL_CFG |= bSW_RESET;

    while(1) {
    }
}

//////////////////////////////////////////////////////////////////////

void goto_bootloader()
{
    led_flash_n_times(BOOTLOADER_FLASH_LED_COUNT, BOOTLOADER_FLASH_LED_SPEED);

    shutdown_peripherals();

    ((void (*)())BOOT_LOAD_ADDR)();    // jump to bootloader
}

//////////////////////////////////////////////////////////////////////
// read bytes from the data flash area

void read_flash_data(uint8 flash_addr, uint8 len, uint8 *dst_data)
{
    flash_addr <<= 1;

    while(len-- != 0) {

        ROM_ADDR_L = flash_addr;
        ROM_ADDR_H = DATA_FLASH_ADDR >> 8;

        ROM_CTRL = ROM_CMD_READ;

        if((ROM_STATUS & bROM_CMD_ERR) != 0) {
            break;
        }

        *dst_data++ = ROM_DATA_L;

        flash_addr += 2;
    }
}

//////////////////////////////////////////////////////////////////////
// write bytes to the data flash area

void write_flash_data(uint8 flash_addr, uint8 len, uint8 *src_data)
{
    flash_addr <<= 1;

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;

    GLOBAL_CFG |= bDATA_WE;

    while(len-- != 0) {

        ROM_ADDR_L = flash_addr;
        ROM_ADDR_H = DATA_FLASH_ADDR >> 8;

        ROM_DATA_L = *src_data++;

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
