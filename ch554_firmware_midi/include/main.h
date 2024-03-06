#pragma once

#define FIRMWARE_VERSION 0x0100

#define BOOTLOADER_FLASH_LED_COUNT 20
#define BOOTLOADER_FLASH_LED_SPEED 0x80

#define BOOT_FLASH_LED_COUNT 10
#define BOOT_FLASH_LED_SPEED 0x40

#define BOOTLOADER_BUTTON_DELAY 30000    // about 10 seconds
// #define BOOTLOADER_BUTTON_DELAY 0x80

#define TIMER0_FREQ 1000

#define TIMER0_PERIOD (2000000 / TIMER0_FREQ)

#define TIMER0_LOW ((uint8)(255 - (TIMER0_PERIOD % 256)))
#define TIMER0_HIGH ((uint8)(255 - (TIMER0_PERIOD / 256)))


void led_set_flash();

void timer0_irq_handler(void) __interrupt(INT_NO_TMR0);