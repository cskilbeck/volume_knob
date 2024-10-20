#pragma once

//////////////////////////////////////////////////////////////////////

extern uint8 led_brightness;

//////////////////////////////////////////////////////////////////////

inline void led_flash()
{
    led_brightness = 255u;
}

//////////////////////////////////////////////////////////////////////

void led_init();
void led_on_tick();
void led_update();

void led_flash_n_times(uint8 n, uint16 speed);
