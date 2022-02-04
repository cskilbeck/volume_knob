#include "main.h"
#include "usb_device.h"
#include "usbd_hid.h"
#include "user.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

//////////////////////////////////////////////////////////////////////

inline void divmod(uint32 val, uint32 denom, uint32 &div, uint32 &mod)
{
#if defined(__CC_ARM)
    __asm("UDIV div,val,denom");       // div = val / denom
    __asm("MLS mod,denom,div,val");    // mod = val % denom (val - (div * denom))
#else
    asm("UDIV %0,%1,%2\n" : "=r"(div) : "r"(val), "r"(denom));
    asm("MLS %0,%1,%2,%3" : "=r"(mod) : "r"(denom), "r"(div), "r"(val));
#endif
}

//////////////////////////////////////////////////////////////////////
// swap low/high 16bits within a 32bit word

inline uint32 swap_32_16(uint32 x)
{
    uint32 sh = 16;
#if defined(__CC_ARM)
    __asm("ROR x,x,sh");    // div = val / denom
#else
    asm("ROR %0,%0,%1\n" : "=r"(x) : "r"(sh));
#endif
    return x;
}


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

struct event
{
    uint8 key;      // set key1 to this and send usb packet
    uint8 delay;    // wait this long to look at the next entry in the queue please
};

queue<event, 32> keys;

uint32 ticks = 0;
uint32 ticks2 = 0;

void update_tick()
{
    ticks += 1;
}

typedef struct keyboard_report
{
    uint8 modifiers;
    uint8 reserved;
    uint8 key1;
    uint8 key2;
    uint8 key3;
    uint8 key4;
    uint8 key5;
    uint8 key6;
} keyboard_report;

keyboard_report keyboard_data = { 0 };

extern USBD_HandleTypeDef hUsbDeviceFS;

static int state = 0;
uint8 store = 0;

//////////////////////////////////////////////////////////////////////
// Valid transitions are:
// 1    00 .. 01
// 2    00 .. 10
// 4    01 .. 00
// 7    01 .. 11
// 8    10 .. 00
// 11   10 .. 11
// 13   11 .. 01
// 14   11 .. 10

// bitmask of which 2-state histories are valid (see table above)
#define valid_state_mask 0x6996

// then, to just get one increment per cycle:

// 11 .. 10 .. 00 is one way
// 00 .. 10 .. 11 is the other way

// So:
// E8 = 11,10 .. 10,00  --> one way
// 2B = 00,10 .. 10,11  <-- other way

int rotary_delta = 0;

void read_encoder()
{
    int a = GPIOB->IDR & 1;
    int b = GPIOB->IDR & 2;

    state = ((state << 2) | (a | b)) & 0xf;

    // many states are invalid (noisy switches) so ignore them
    if((valid_state_mask & (1 << state)) != 0) {
        // certain state patterns mean rotation happened
        store = (store << 4) | state;
        switch(store) {
        case 0xe8:
            rotary_delta += 1;
            break;
        case 0x2b:
            rotary_delta -= 1;
        }
    }
}

int button_history = 0xffff;
bool button_state = false;
bool button_prev = false;
bool button_press = false;
bool button_release = false;

void button_update()
{
    LL_TIM_ClearFlag_UPDATE(TIM2);

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

// there is a circular queue of keys to send
// if delay timer == 0
//      if queue !empty
//          get key at head of queue
//          send key up usb event
// else if queue !empty
//      get key at head of queue
//      send key down (volume up/down) usb event
//      set delay timer to 2 ms

// set can_send_mute = true
// loop:
//     wait for an interrupt
//     if rotary changed, set key1 = a volume key and set timeout
//     if button pressed, set key1 = mute toggle key and set timeout
//     if timeout expired, clr key1

uint32 expiry = 0;

#define key_mute 4 //127
#define key_volume_up 5 //128
#define key_volume_down 6 //129

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

        int d = rotary_delta;
        rotary_delta = 0;
        
        // if timer has expired
        if(!keys.empty() && ticks2 > expiry) {
            event e = keys.remove();
            keyboard_data.key1 = e.key;
            USBD_HID_SendReport(&hUsbDeviceFS, (uint8 *)&keyboard_data, sizeof(keyboard_data));
            expiry = e.delay * 100; // delay is in 1/10ths of a second
            ticks2 = 0;
        }

        if(d != 0 ) {
            add_key(d == 1 ? key_volume_up : key_volume_down);
        }
        
        if(button_press) {
            add_key(key_mute);
            button_press = 0;
        }
        
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

        //        if(button_press) {
        //
        //            GPIOC->BSRR = 1 << (13 + 16);
        //            // set key2 = mute

        //        } else if(button_release) {

        //            GPIOC->BSRR = 1 << 13;
        //            // set key2 = 0
        //        }
        //        int x = rotary_delta;
        //        if(x < 0) {
        //        } else if(x > 0) {
        //        }
    }
}
