#include "main.h"

//////////////////////////////////////////////////////////////////////

// Endpoint0 OUT & IN
__xdata uint8 Ep0Buffer[DEFAULT_ENDP0_SIZE];
__xdata uint8 Ep1Buffer[DEFAULT_ENDP1_SIZE];
__xdata uint8 Ep2Buffer[USB_PACKET_SIZE * 2];
__xdata uint8 serial_number_string[SERIAL_STRING_LEN];    // e.g "012345678"
__xdata uint8 product_string[PRODUCT_NAME_STRING_LEN];    // e.g. "Tiny Midi Knob 012345678"

//////////////////////////////////////////////////////////////////////

#define MEDIA_KEY(x) ((x) | 0x8000)
#define NORMAL_KEY(x) (x)

//////////////////////////////////////////////////////////////////////

volatile uint8 tick = 0;

void timer0_irq_handler(void) __interrupt(INT_NO_TMR0)
{
    TL0 = TIMER0_LOW;
    TH0 = TIMER0_HIGH;
    tick = 1;
}

//////////////////////////////////////////////////////////////////////
// a queue of key states to send

// must be a power of 2
#define KEY_QUEUE_LEN 16

uint16 queue_buffer[KEY_QUEUE_LEN];
uint8 queue_head = 0;
uint8 queue_size = 0;

inline bool queue_full()
{
    return queue_size == KEY_QUEUE_LEN;
}

inline uint8 queue_space()
{
    return KEY_QUEUE_LEN - queue_size;
}

inline bool queue_empty()
{
    return queue_size == 0;
}

// check it's got space before calling this

void queue_put(uint16 k)
{
    queue_buffer[(queue_head + queue_size) & (KEY_QUEUE_LEN - 1)] = k;
    queue_size += 1;
}

// check it's not empty before calling this

uint16 queue_get()
{
    uint16 next = queue_buffer[queue_head];
    queue_size -= 1;
    queue_head = ++queue_head & (KEY_QUEUE_LEN - 1);
    return next;
}

//////////////////////////////////////////////////////////////////////
// add a momentary keypress to the queue and flash the led

void do_press(uint16 k)
{
    if(queue_space() > 1) {
        queue_put(k);
        queue_put(k & 0x8000);
        led_flash();
    } else {

        print_uint8("Space", queue_space());
    }
}

//////////////////////////////////////////////////////////////////////

#define DEBOUNCE_TIME 0x1Cu
#define T2_DEBOUNCE (0xFFu - DEBOUNCE_TIME)

uint16 press_time = 0;

uint8 vol_direction;
int8 turn_value;

__code uint8 const product_name[] = PRODUCT_NAME;

void main()
{
    // init clock
    clk_init();
    delay_mS(5);

#if DEVICE == DEVICE_DEVKIT
    gpio_init(UART_TX_PORT, UART_TX_PIN, gpio_output_push_pull);
    gpio_init(UART_RX_PORT, UART_RX_PIN, gpio_output_open_drain);
    uart0_init();
#endif

    putstr("================ BOOT =================\n");

    init_chip_id();

    print_uint32("CHIP ID", chip_id);

    // insert chip id into serial string and product name string

    product_string[0] = PRODUCT_NAME_STRING_LEN;
    product_string[1] = USB_DESCR_TYP_STRING;
    for(uint8 i = 0; i < PRODUCT_NAME_LEN; ++i) {
        product_string[2 + i * 2] = product_name[i];
        product_string[3 + i * 2] = 0;
    }

    serial_number_string[0] = SERIAL_STRING_LEN;
    serial_number_string[1] = USB_DESCR_TYP_STRING;
    uint32 n = chip_id;
    for(uint8 i = 0; i < SERIAL_LEN; ++i) {
        uint8 c = (uint8)(n >> 28);    // @hardcoded
        if(c < 10) {
            c += '0';
        } else {
            c += 'A' - 10;
        }
        serial_number_string[2 + i * 2] = c;
        serial_number_string[3 + i * 2] = 0;
        product_string[2 + i * 2 + PRODUCT_NAME_LEN * 2] = c;
        product_string[3 + i * 2 + PRODUCT_NAME_LEN * 2] = 0;
        n <<= 4;
    }

    hexdump("SERIAL", serial_number_string, SERIAL_STRING_LEN);
    hexdump("PRODUCT", product_string, PRODUCT_NAME_STRING_LEN);

    gpio_init(ROTA_PORT, ROTA_PIN, gpio_input_pullup);
    gpio_init(ROTB_PORT, ROTB_PIN, gpio_input_pullup);
    gpio_init(BTN_PORT, BTN_PIN, gpio_input_pullup);
    gpio_init(LED_PORT, LED_PIN, gpio_output_push_pull);

    read_flash_data(0, 1, &vol_direction);

    switch(vol_direction) {
    case 0:
        turn_value = 1;
        break;
    case 2:
        turn_value = 1;
        break;
    default:
        vol_direction = ROTARY_DIRECTION;
        turn_value = ROTARY_DIRECTION - 1;
        break;
    }
    print_uint8("TURN", turn_value);

    usb_device_config();
    usb_device_endpoint_config();    // Endpoint configuration
    usb_device_int_config();         // Interrupt initialization
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;    // Be pre -use and sending length must be empty
    UEP2_T_LEN = 0;    // Be pre -use and sending length must be empty

    // setup timers

    // timer2 uses defaults
    TL2 = 0;
    TH2 = 0;
    TF2 = 0;

    // timer0 16 bit mode
    TMOD = (TMOD & ~MASK_T0_MOD) | 0x01;

    TL0 = TIMER0_LOW;
    TH0 = TIMER0_HIGH;

    // clear overflow flag
    TF0 = 0;

    // enable irq
    ET0 = 1;

    // enable the timers
    TR0 = 1;    // 1 KHz tick
    TR1 = 1;    // UART
    TR2 = 1;    // debounce

    // global irq enable
    EA = 1;

    flash_led(BOOT_FLASH_LED_COUNT, BOOT_FLASH_LED_SPEED);

    // Triple click admin
    uint16 t1_count = 0;
    uint8 clicks = 0;

    // flash led slowly until USB is connected (in case of power-only cable)

    {
        uint8_t flash = 0;

        while(!usb_active) {

            if(TF2 == 1) {
                TL2 = 0;
                TH2 = 0;
                TF2 = 0;
                flash += 1;
                LED_BIT = (flash >> 4) & 1;
            }
        }
    }

    // main loop

    bool button_state = false;    // for debouncing the button

    while(1) {

        // read/debounce the button
        bool pressed = false;
        bool new_state = !BTN_BIT;
        if(new_state != button_state && TF2 == 1) {
            TL2 = 0;
            TH2 = T2_DEBOUNCE;
            TF2 = 0;
            pressed = new_state;
            button_state = new_state;
        }

        if(tick) {
            tick = 0;
            if(button_state) {
                press_time += 1;
                if(press_time == BOOTLOADER_BUTTON_DELAY) {
                    goto_bootloader();
                }
            } else {
                press_time = 0;
            }

            // 400ms delay counter for knob triple-click
            if(t1_count < 400) {
                t1_count += 1;
            } else {
                clicks = 0;
            }
            led_tick();
        }

        // read the rotary encoder (returns -1, 0 or 1)
        int8 direction = read_encoder();

        // Timer2 does double duty, debounces encoder as well
        if(direction != 0 && TF2 == 1) {
            TL2 = 0;
            TH2 = T2_DEBOUNCE;
            TF2 = 0;
        } else {
            direction = 0;
        }

        // check for triple-click
        if(pressed) {
            if(t1_count < 400) {
                clicks += 1;
                if(clicks == 2) {
                    vol_direction = 2 - vol_direction;
                    turn_value = (int8)vol_direction - 1;
                    write_flash_data(0, 1, &vol_direction);
                    pressed = false;
                }
            }
            t1_count = 0;
        }

        // queue up some keypresses if something happened
        if(pressed) {

            do_press(KEY_1);
        }

        if(direction == turn_value) {

            do_press(MEDIA_KEY(KEY_MEDIA_VOLUMEUP));

        } else if(direction == -turn_value) {

            do_press(MEDIA_KEY(KEY_MEDIA_VOLUMEDOWN));
        }

        // send key on/off to usb hid if there are some waiting to be sent
        if((usb_idle & 2) != 0 && !queue_empty()) {

            usb_set_keystate(queue_get());
        }
        led_update();
    }
}
