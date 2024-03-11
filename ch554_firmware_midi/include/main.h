#pragma once

#define FIRMWARE_CURRENT_VERSION 0x010000

#define FIRMWARE_VERSION ((FIRMWARE_CURRENT_VERSION << 8) | DEVICE)

#define BOOTLOADER_FLASH_LED_COUNT 20
#define BOOTLOADER_FLASH_LED_SPEED 0x80

#define BOOT_FLASH_LED_COUNT 10
#define BOOT_FLASH_LED_SPEED 0x40

#define BOOTLOADER_BUTTON_DELAY 30000    // about 30 seconds
// #define BOOTLOADER_BUTTON_DELAY 0x80

#define TIMER0_FREQ 1000

#define TIMER0_PERIOD (2000000 / TIMER0_FREQ)

#define TIMER0_LOW ((uint8)(255 - (TIMER0_PERIOD % 256)))
#define TIMER0_HIGH ((uint8)(255 - (TIMER0_PERIOD / 256)))

#define SERIAL_LEN 8    // 01234567
#define SERIAL_STRING_LEN (2 + 2 * SERIAL_LEN)

#define PRODUCT_NAME "Tiny Midi Knob "
#define PRODUCT_NAME_LEN 15
#define PRODUCT_NAME_STRING_LEN (2 + 2 * (PRODUCT_NAME_LEN + SERIAL_LEN))

void led_set_flash();

void timer0_irq_handler(void) __interrupt(INT_NO_TMR0);