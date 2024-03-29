#include "main.h"

#define BOOT_FLASH_LED_COUNT 10
#define BOOT_FLASH_LED_SPEED 20

void main()
{
    clk_init();
    tick_init();
    uart0_init();
    led_init();
    encoder_init();

    if(BTN_BIT == 0) {    // if button held down when the device is plugged in, go to bootloader
        goto_bootloader();
    }

    EA = 1;    // global irq enable

    printf(CLEAR_CONSOLE "---------- %s ----------\nCHIP ID: %08lx\n", current_process->process_name, chip_id);

    load_config();
    usb_init();
    led_flash_n_times(BOOT_FLASH_LED_COUNT, BOOT_FLASH_LED_SPEED);
    usb_wait_for_connection();

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

        if(pressed) {
            current_process->on_press();
        }

        if(released) {
            current_process->on_release();
        }

        int8 direction = encoder_read();
        if(direction != 0) {
            current_process->on_rotate(direction);
        }

        for(uint8 i = 0; i < num_endpoints; ++i) {    // process or discard any incoming USB data
            uint8 got = usb.recv_len[i];
            if(got != 0 && current_process->on_usb_receive[i] != NULL) {
                current_process->on_usb_receive[i](got);
            }
            usb.recv_len[i] = 0;
        }

        current_process->on_update();

        led_update();
    }
}
