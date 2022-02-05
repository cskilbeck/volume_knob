//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "usb_device.h"
#include "usbd_hid.h"
#include "user.h"
#include "rotary.h"
#include "queue.h"

//////////////////////////////////////////////////////////////////////

#define KEY_MEDIA_MUTE 0xE2
#define KEY_MEDIA_VOL_UP 0xE9
#define KEY_MEDIA_VOL_DOWN 0xEA

//////////////////////////////////////////////////////////////////////
// events are added to the queue and cause usb events at reasonable intervals

enum class event_type : uint8
{
    key = 0,
    media = 1
};

struct event
{
    uint8 key;          // set key1 to this and send usb packet
    uint8 delay;        // wait this long to look at the next entry in the queue please
    event_type type;    // it's a key or a media key
    uint8 pad;
};

//////////////////////////////////////////////////////////////////////
// keyboard event has slots for 6 keys and the modifiers (shift etc)

struct keyboard_report
{
    uint8 modifiers;
    uint8 reserved;
    uint8 key1;
    uint8 key2;
    uint8 key3;
    uint8 key4;
    uint8 key5;
    uint8 key6;
};

//////////////////////////////////////////////////////////////////////

extern USBD_HandleTypeDef hUsbDeviceFS;

queue<event, 32> keys = { 0 };

uint32 ticks10khz = 0;
uint32 delay_start = 0;
uint32 delay_length = 0;
bool led_flashing = false;
uint32 led_flash_start = 0;
uint32 led_flash_time = 0;

keyboard_report keyboard_data = { 0 };

int button_history = 0xffff;
bool button_state = false;
bool button_prev = false;
bool button_press = false;
bool button_release = false;

//////////////////////////////////////////////////////////////////////

void reset_usb()
{
    // disable usb
    USBD_DeInit(&hUsbDeviceFS);
    
    // set A12, A11 as outputs
    uint32 const pins = (1<<11) | (1<<12);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pins;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // set them to low
    GPIOA->BSRR = pins << 16;

    // wait 50ms
    uint32 now = ticks10khz;
    while((ticks10khz - now) < 500) {
        __nop();
    }

    // switch usb back on
    MX_USB_DEVICE_Init();
}

//////////////////////////////////////////////////////////////////////

void flash_led(int ms)
{
    led_flash_time = ms * 10;
    led_flash_start = ticks10khz;
    GPIOC->BSRR = 1 << (13 + 16);
    led_flashing = true;
}

//////////////////////////////////////////////////////////////////////
// call this in timer 2 isr (10KHz) to update button status

void button_update()
{
    ticks10khz += 1;

    // get current button state (0 = pressed, 1 = released)
    
    int b = (GPIOA->IDR >> 7) & 1;

    // how much debouncing
    
    int const state_count = 12;
    uint32 const state_mask = (1 << state_count) - 1;
    uint32 const press_pattern = state_mask & (~1);
    uint32 const release_pattern = state_mask >> 1;

    // add to record of last N states
    
    button_history = ((button_history << 1) | b) & state_mask;

    // lots of offs followed by an on = a press
    
    if(button_history == press_pattern) {

        button_state = true;

    // or lots of ons followed by an off = a release
        
    } else if(button_history == release_pattern) {

        button_state = false;
    }

    // then press/release admin
    
    bool button_change = button_prev != button_state;
    button_prev = button_state;
    button_press = button_state && button_change;
    button_release = !button_state && button_change;

    if(led_flashing && (ticks10khz - led_flash_start) > led_flash_time) {
        GPIOC->BSRR = 1 << 13;
        led_flashing = false;
    }
}

//////////////////////////////////////////////////////////////////////
// add a keyboard event to the queue for sending when it can

void add_key_event(uint8 k, event_type ev_type)
{
    // room for 2 events? (key down, key up)

    if(keys.space() >= 2) {

        event e;

        // add the key down event

        e.key = k;
        e.delay = 10;
        e.type = ev_type;
        keys.add(e);
        
        // add the key up event

        e.key = 0;
        e.delay = 10;
        e.type = ev_type;
        keys.add(e);

        flash_led(20);
    }
}

//////////////////////////////////////////////////////////////////////

void user_main()
{
    GPIOC->BSRR = 1 << 13;
    
    LL_TIM_EnableIT_UPDATE(TIM2);
    LL_TIM_EnableCounter(TIM2);
    LL_TIM_EnableARRPreload(TIM2);

    reset_usb();
    
    // start timer 2 for 10KHz ticker

    while(true) {

        //__WFI();

        // nab any changes which have happened in interrupts
        
        __disable_irq();
        int knob_movement = rotary_delta;
        bool button_hit = button_press;
        rotary_delta = 0;
        button_press = false;
        __enable_irq();

        // if timer has expired

        if(!keys.empty() && (ticks10khz - delay_start) > delay_length) {

            // process next key event

            event e = keys.remove();
            uint8_t report[3];

            switch(e.type) {

            // normal keyboard up or down

            case event_type::key:
                keyboard_data.key1 = e.key;
                USBD_HID_SendReport(&hUsbDeviceFS, (uint8 *)&keyboard_data, sizeof(keyboard_data));
                break;

            // media key up or down

            case event_type::media:
                report[0] = 2;
                report[1] = e.key;
                report[2] = 0;
                USBD_HID_SendReport(&hUsbDeviceFS, report, 3);
                break;
            }
            
            // set delay so next event is delayed by N milliseconds

            delay_start = ticks10khz;
            delay_length = e.delay * 10;
        }

        // if knob was twisted, send volume up or down
        
        switch(knob_movement) {
            case -1:
                add_key_event(KEY_MEDIA_VOL_UP, event_type::media);
                break;
            case 1:
                add_key_event(KEY_MEDIA_VOL_DOWN, event_type::media);
                break;
        }

        // if knob was pressed, send mute
        
        if(button_hit) {
            add_key_event(KEY_MEDIA_MUTE, event_type::media);
        }
    }
}
