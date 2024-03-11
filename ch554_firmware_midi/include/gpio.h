#pragma once

//////////////////////////////////////////////////////////////////////

typedef enum gpio_state
{
    gpio_input_no_pullup = 0,
    gpio_output_push_pull = 1,
    gpio_output_open_drain = 2,
    gpio_input_pullup = 3
} gpio_state_t;

void gpio_init(uint8 port, uint8 bit, gpio_state_t state);
