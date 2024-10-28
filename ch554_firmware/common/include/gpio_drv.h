#pragma once

//////////////////////////////////////////////////////////////////////

#define PORT1 0x90
#define PORT3 0xB0

#if DEVICE == DEVICE_ORIGINAL

// USB Micro B version

#define ROTA_PORT PORT3
#define ROTB_PORT PORT3
#define BTN_PORT PORT3
#define LED_PORT PORT1

#define ROTA_PIN 1
#define ROTB_PIN 0
#define BTN_PIN 2
#define LED_PIN 4

#define LED_ON 0
#define LED_OFF 1

#elif DEVICE == DEVICE_DIRECT

// USB A version

#define ROTA_PORT PORT1
#define ROTB_PORT PORT1
#define BTN_PORT PORT1
#define LED_PORT PORT1

#define ROTA_PIN 6
#define ROTB_PIN 4
#define BTN_PIN 7
#define LED_PIN 5

#define LED_ON 0
#define LED_OFF 1

#elif DEVICE == DEVICE_DEVKIT

// CH554G Devkit version

#define UART_TX_PORT PORT3
#define UART_RX_PORT PORT3
#define ROTA_PORT PORT3
#define ROTB_PORT PORT3
#define BTN_PORT PORT3
#define LED_PORT PORT1

#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define ROTA_PIN 3
#define ROTB_PIN 4
#define BTN_PIN 2
#define LED_PIN 6

#define LED_ON 1
#define LED_OFF 0

#elif DEVICE == DEVICE_ORIGINAL_V2

#define ROTA_PORT PORT1
#define ROTB_PORT PORT1
#define BTN_PORT PORT1
#define LED_PORT PORT1

#define BTN_PIN 4
#define LED_PIN 5
#define ROTA_PIN 6
#define ROTB_PIN 7

#define LED_ON 0
#define LED_OFF 1

#else

#error DEVICE NOT DEFINED!?

#endif

SBIT(ROTA_BIT, ROTA_PORT, ROTA_PIN);
SBIT(ROTB_BIT, ROTB_PORT, ROTB_PIN);
SBIT(BTN_BIT, BTN_PORT, BTN_PIN);
SBIT(LED_BIT, LED_PORT, LED_PIN);

//////////////////////////////////////////////////////////////////////

typedef enum gpio_state
{
    gpio_input_no_pullup = 0,
    gpio_output_push_pull = 1,
    gpio_output_open_drain = 2,
    gpio_input_pullup = 3
} gpio_state_t;

void gpio_init(uint8 port, uint8 bitpos, gpio_state_t state);
