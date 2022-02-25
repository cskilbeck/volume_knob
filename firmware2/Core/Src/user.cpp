//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "usb_device.h"
#include "usbd_desc.h"
#include "usbd_hid.h"
#include "user.h"
#include "rotary.h"
#include "queue.h"
#include "timer.h"

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
    uint8 key;          // what to send
    uint8 delay;        // wait this long before sending the next entry in the queue please
    event_type type;    // it's a key or a media key
    uint8 pad;
};

//////////////////////////////////////////////////////////////////////
// keyboard report has slots for 6 keys and the modifiers (shift etc)

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
// media keys handled differently I guess

struct media_key_report
{
    uint8 device_id;    // set this to 2
    uint8 key;          // media keys defined above
    uint8 pad;          // set this to 0
};

//////////////////////////////////////////////////////////////////////

extern USBD_HandleTypeDef hUsbDeviceFS;

// queue of things to send to the usb host

queue<event, 32> messages = { 0 };

// usb report source buffers

keyboard_report keyboard_report = { 0 };
media_key_report media_report = { 0 };

// 10KHz tick count

uint32 ticks10khz = 0;

// admin for waiting between messages

timer msg_delay;

// led flash admin

bool led_flashing = false;
timer led_flash_timer;

// button debounce admin

int button_history = 0xffff;
bool button_state = false;
bool button_prev = false;
bool button_press = false;
bool button_release = false;

//////////////////////////////////////////////////////////////////////

extern "C" void EXTI0_1_IRQHandler()
{
    if((EXTI->PR & (1 << 0)) != 0) {
        EXTI->PR = (1 << 0);
        read_encoder();
    }
}

//////////////////////////////////////////////////////////////////////

extern "C" void EXTI4_15_IRQHandler()
{
    if((EXTI->PR & (1 << 8)) != 0) {
        EXTI->PR = (1 << 8);
        read_encoder();
    }
}

//////////////////////////////////////////////////////////////////////

extern "C" void TIM14_IRQHandler()
{
    TIM14->SR = ~TIM_SR_UIF;
    update_10khz();
}

//////////////////////////////////////////////////////////////////////

void wait(int ms)
{
    timer t(ms * 10);
    while(!t.expired()) {
        __WFI();
    }
}

//////////////////////////////////////////////////////////////////////

void reset_usb()
{
    // disable usb
    USBD_DeInit(&hUsbDeviceFS);

    // set A12, A11 as outputs
    uint32 const pins = (1 << 11) | (1 << 12);

    LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = pins;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // set them to low
    GPIOA->BSRR = pins << 16;

    // wait 50ms
    wait(50);

    // switch usb back on (seems to steal the GPIOs itself)
    MX_USB_DEVICE_Init();
}

//////////////////////////////////////////////////////////////////////

void flash_led(int ms)
{
    LED_GPIO_Port->BSRR = LED_Pin << 16;
    led_flash_timer.reset(ms * 10);
    led_flashing = true;
}

//////////////////////////////////////////////////////////////////////
// call this in timer 2 isr (10KHz) to update button status & led

void update_10khz()
{
    ticks10khz += 1;

    // get current button state (0 = pressed, 1 = released)

    int b = (BTN_GPIO_Port->IDR >> BIT_POS(BTN_Pin)) & 1;

    // how much debouncing

    uint32 const state_count = 12;
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

    // debug led flashing admin

    if(led_flashing && led_flash_timer.expired()) {
        LED_GPIO_Port->BSRR = LED_Pin;
        led_flashing = false;
    }
}

//////////////////////////////////////////////////////////////////////
// add a keyboard event to the queue for sending when it can

void add_key_event(uint8 k, event_type ev_type)
{
    // room for 2 events? (key down, key up)

    if(messages.space() >= 2) {
        
        // if queue was empty, send it straight away
        
        if(messages.empty()) {
            msg_delay.reset(0);
        }

        event e;

        // add the key down event

        e.key = k;
        e.delay = 100;
        e.type = ev_type;
        messages.add(e);

        // add the key up event

        e.key = 0;
        e.delay = 100;
        e.type = ev_type;
        messages.add(e);

        flash_led(20);
    }
}

//////////////////////////////////////////////////////////////////////

void user_main()
{
    // set USB class pointers etc but don't switch it on

    USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);
    USBD_RegisterClass(&hUsbDeviceFS, &USBD_HID);

    // kill systick

    HAL_NVIC_DisableIRQ(SysTick_IRQn);

    // start timer 14 for 10KHz ticker

    LL_TIM_EnableARRPreload(TIM14);
    LL_TIM_EnableIT_UPDATE(TIM14);
    LL_TIM_EnableCounter(TIM14);

    // enable all the IRQs

    NVIC_EnableIRQ(TIM14_IRQn);
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_EnableIRQ(EXTI4_15_IRQn);

    // flash LED for 100 ms at boot

    flash_led(100);

    // start usb enumeration

    reset_usb();

    // main loop

    while(true) {

        // idle until an interrupt has occurred

        __WFI();

        // nab any changes which have happened in interrupts

        __disable_irq();
        int knob_movement = rotary_delta;
        bool button_hit = button_press;
        rotary_delta = 0;
        button_press = false;
        __enable_irq();

        // if events are ready and waiting to be sent

        if(!messages.empty() && msg_delay.expired()) {

            // process next key event

            event e = messages.remove();

            switch(e.type) {

                // normal keyboard up or down

            case event_type::key:
                keyboard_report.key1 = e.key;
                USBD_HID_SendReport(&hUsbDeviceFS, (uint8 *)&keyboard_report, sizeof(keyboard_report));
                break;

                // media key up or down

            case event_type::media:
                media_report.device_id = 2;
                media_report.key = e.key;
                USBD_HID_SendReport(&hUsbDeviceFS, (uint8 *)&media_report, sizeof(media_report));
                break;
            }

            // set delay so next event is delayed by N milliseconds

            msg_delay.reset(e.delay);
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
