//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "usb_device.h"
#include "usbd_hid.h"
#include "user.h"
#include "rotary.h"

//////////////////////////////////////////////////////////////////////

extern USBD_HandleTypeDef hUsbDeviceFS;

#define key_mute 4 //127
#define key_volume_up 5 //128
#define key_volume_down 6 //129

//////////////////////////////////////////////////////////////////////

template <typename T, int N> struct queue
{
    uint16 pos;
    uint16 len;

    T q[N];

    bool empty() const
    {
        return len == 0;
    }

    bool full() const
    {
        return len == N;
    }
    
    int space() const
    {
        return N - len;
    }

    // add new to the front of the queue

    void add(T const &item)
    {
        q[(pos + len) % N] = item;
        len += 1;
    }

    // get old from the back of the queue

    T remove()
    {
        int x = pos;
        len -= 1;
        pos = (pos + 1) % N;
        return q[x];
    }
};

//////////////////////////////////////////////////////////////////////

struct event
{
    uint8 key;      // set key1 to this and send usb packet
    uint8 delay;    // wait this long to look at the next entry in the queue please
};

//////////////////////////////////////////////////////////////////////

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

queue<event, 32> keys = { 0 };

uint32 ticks = 0;
uint32 ticks2 = 0;
uint32 expiry = 0;

//////////////////////////////////////////////////////////////////////

keyboard_report keyboard_data = { 0 };

//////////////////////////////////////////////////////////////////////

int button_history = 0xffff;
bool button_state = false;
bool button_prev = false;
bool button_press = false;
bool button_release = false;

//////////////////////////////////////////////////////////////////////

void update_tick()
{
    ticks += 1;
}

//////////////////////////////////////////////////////////////////////

void button_update()
{
    ticks2 += 1;

    int b = (GPIOA->IDR >> 7) & 1;

    button_history = ((button_history << 1) | b) & 0xff;

    if(button_history == 0xfe) {

        button_state = true;

    } else if(button_history == 0x7f) {

        button_state = false;
    }

    bool button_change = button_prev != button_state;
    button_prev = button_state;
    button_press = button_state && button_change;
    button_release = !button_state && button_change;
}

//////////////////////////////////////////////////////////////////////

void add_key(uint8 k)
{
    if(keys.space() > 1) {
        event key_down;
        key_down.key = k;
        key_down.delay = 10;
        keys.add(key_down);

        event key_up;
        key_up.key = 0;
        key_up.delay = 10;
        keys.add(key_up);
    }
}

//////////////////////////////////////////////////////////////////////

void user_main()
{
    LL_TIM_EnableIT_UPDATE(TIM2);
    LL_TIM_EnableCounter(TIM2);
    LL_TIM_EnableARRPreload(TIM2);

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOC);
    LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = LL_GPIO_PIN_13;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_MODE_OUTPUT_50MHz;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIOC->BSRR = 1 << (13 + 16);

    int z = 0;
    int rotary_value = 0;

    while(true) {

        //__WFI();

        __disable_irq();
        int d = rotary_delta;
        int b = button_press;
        rotary_delta = 0;
        button_press = 0;
        __enable_irq();
        
//        // if timer has expired
//        if(!keys.empty() && ticks2 > expiry) {
//            event e = keys.remove();
//            keyboard_data.key1 = e.key;
//            USBD_HID_SendReport(&hUsbDeviceFS, (uint8 *)&keyboard_data, sizeof(keyboard_data));
//            expiry = e.delay * 100; // delay is in 1/10ths of a second
//            ticks2 = 0;
//        }

//        if(d != 0 ) {
//            
//            add_key(d == 1 ? key_volume_up : key_volume_down);
//        }
//        
//        if(button_press) {
//            add_key(key_mute);
//            button_press = 0;
//        }
        
        rotary_value += d;

        int const bits = 12;
        int const r = (1 << bits) - 1;

        z = (rotary_value * 512) & r;
        int x = (z * z) >> bits;

        GPIOC->BSRR = 1 << (13 + 16);
        for(volatile int i = 0; i < x; ++i) {
            __nop();
        }
        GPIOC->BSRR = 1 << 13;
        x = r - x;
        for(volatile int i = 0; i < x; ++i) {
            __nop();
        }
    }
}
