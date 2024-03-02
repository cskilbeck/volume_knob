#pragma once

//////////////////////////////////////////////////////////////////////

#define PORT1 0x90
#define PORT3 0xB0

#define UART_TX_PORT PORT3
#define UART_TX_PIN 0

#define UART_RX_PORT PORT3
#define UART_RX_PIN 1

#define ROTA_PORT PORT3
#define ROTA_PIN 3

#define ROTB_PORT PORT3
#define ROTB_PIN 4

#define BTN_PORT PORT3
#define BTN_PIN 2

#define LED_PORT PORT1
#define LED_PIN 6

SBIT(BTN_BIT, BTN_PORT, BTN_PIN);
SBIT(LED_BIT, LED_PORT, LED_PIN);
SBIT(ROTA_BIT, ROTA_PORT, ROTA_PIN);
SBIT(ROTB_BIT, ROTB_PORT, ROTB_PIN);

//////////////////////////////////////////////////////////////////////

typedef enum gpio_state
{
    gpio_input_no_pullup = 0,
    gpio_output_push_pull = 1,
    gpio_output_open_drain = 2,
    gpio_input_pullup = 3
} gpio_state_t;

void gpio_init(uint8 port, uint8 bit, gpio_state_t state);