//////////////////////////////////////////////////////////////////////

#include "main.h"

uint32 chip_id;
uint32 chip_id_28;

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

//////////////////////////////////////////////////////////////////////
// Flash LED

void flash_led(uint8 n, uint8 speed)
{
    LED_BIT = LED_OFF;
    while(n-- != 0) {
        TH2 = speed;
        TL2 = 0;
        while(TF2 != 1) {
        }
        TF2 = 0;
        LED_BIT ^= 1;
    }
    LED_BIT = LED_OFF;
}

//////////////////////////////////////////////////////////////////////

void goto_bootloader()
{
    // shutdown peripherals
    EA = 0;
    USB_CTRL = 0;
    UDEV_CTRL = 0;

    // flash the led a few times
    flash_led(BOOTLOADER_FLASH_LED_COUNT, BOOTLOADER_FLASH_LED_SPEED);

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
