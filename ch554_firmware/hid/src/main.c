#include "main.h"

//////////////////////////////////////////////////////////////////////
// TIMER0 = LED pwm
// TIMER1 = UART
// TIMER2 = 1KHz tick

//////////////////////////////////////////////////////////////////////

#define CLEAR_CONSOLE "\033c\033[3J\033[2J"

void main()
{
    clk_init();
    tick_init();
    uart0_init();
    led_init();
    encoder_init();

    // if button held down when the device is plugged in, go to bootloader
    if(BTN_BIT == 0) {
        goto_bootloader();
    }

    // global irq enable
    EA = 1;

    printf(CLEAR_CONSOLE "---------- %s ----------\nCHIP ID: %08lx\n", current_process->process_name, chip_id);

    load_config();

    usb_init();

    led_flash_n_times(BOOT_FLASH_LED_COUNT, BOOT_FLASH_LED_SPEED);

    usb_wait_for_connection();

    puts("Main loop");

    current_process->on_init();

    bool button_state = false;
    uint8 button_debounce_ticks = 0;

    while(1) {

        bool pressed = false;
        bool released = false;
        bool new_state = !BTN_BIT;

        if(new_state != button_state && button_debounce_ticks > 1) {
            button_debounce_ticks = 0;
            pressed = new_state;
            released = !new_state;
            button_state = new_state;
        }

        if(TF2) {    // tick
            TF2 = 0;
            if(button_debounce_ticks <= 2) {
                button_debounce_ticks += 1;
            }
            current_process->on_tick();
            led_on_tick();
        }

        // read the rotary encoder (returns -1, 0 or 1)
        int8 direction = encoder_read();

        if(pressed) {

            current_process->on_press();
        }

        if(released) {

            current_process->on_release();
        }

        if(direction != 0) {

            current_process->on_rotate(direction);
        }

        // process or discard any incoming USB data

        for(uint8 i = 1; i < 4; ++i) {
            uint8 got = usb.recv_len[i];
            if(got != 0 && current_process->on_usb_receive[i - 1] != NULL) {
                current_process->on_usb_receive[i - 1](got);
            }
            usb.recv_len[i] = 0;
        }

        current_process->on_update();

        led_update();
    }
}
