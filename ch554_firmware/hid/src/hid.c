#include "main.h"

#define XDATA __xdata
#include "xdata_extra.h"
#undef XDATA

//////////////////////////////////////////////////////////////////////
// a queue of events

typedef enum event
{
    event_clockwise = 1,
    event_counterclockwise = 2,
    event_press = 3,

    event_mask = 0x3f,

    event_keydown = 0,     // key down is on by default
    event_keyup = 0x80,    // else key up

} event_t;

//////////////////////////////////////////////////////////////////////

void hid_process_event(event_t e)
{
    uint8 is_keyup = (e & event_keyup) != 0;
    uint16 key = 0;
    uint8 modifiers = 0;

    switch(e & event_mask) {

    case event_clockwise:
        key = hid_config.key_clockwise;
        modifiers = hid_config.mod_clockwise;
        break;

    case event_counterclockwise:
        key = hid_config.key_counterclockwise;
        modifiers = hid_config.mod_counterclockwise;
        break;

    case event_press:
        key = hid_config.key_press;
        modifiers = hid_config.mod_press;
        break;
    }

    if(IS_CONSUMER_KEY(key)) {

        consumer_control_hid_report *cc = (consumer_control_hid_report *)usb_endpoint_1_tx_buffer;
        cc->report_id = 0x02;
        cc->keycode = is_keyup ? 0 : (key & 0x7fff);
        usb_send(endpoint_1, sizeof(consumer_control_hid_report));

    } else if(IS_KEYBOARD_KEY(key)) {

        keyboard_hid_report *k = (keyboard_hid_report *)usb_endpoint_1_tx_buffer;
        k->report_id = 0x01;
        k->modifiers = is_keyup ? 0 : modifiers;
        k->pad = 0;
        k->key[0] = is_keyup ? 0 : KEY_VALUE_U8(key);
        k->key[1] = 0;
        k->key[2] = 0;
        k->key[3] = 0;
        k->key[4] = 0;
        k->key[5] = 0;
        usb_send(endpoint_1, sizeof(keyboard_hid_report));

    } else {

        mouse_hid_report *m = (mouse_hid_report *)usb_endpoint_2_tx_buffer;
        m->report_id = 0x03;
        m->buttons = 0;
        m->x = 0;
        m->y = 0;
        m->wheel1 = 0;
        m->wheel2 = 0;
        if(!is_keyup) {
            if(IS_MOUSE_BUTTON(key)) {
                m->buttons = KEY_VALUE_U8(key);
            } else if(IS_MOUSE_WHEEL_V(key)) {
                m->wheel1 = KEY_VALUE_I8(key);
            } else if(IS_MOUSE_WHEEL_H(key)) {
                m->wheel2 = KEY_VALUE_I8(key);
            } else if(IS_MOUSE_MOVE_X(key)) {
                m->x = KEY_VALUE_I8(key);
            } else if(IS_MOUSE_MOVE_Y(key)) {
                m->y = KEY_VALUE_I8(key);
            }
        }
        usb_send(endpoint_2, sizeof(mouse_hid_report));
    }
}

//////////////////////////////////////////////////////////////////////

void hid_init()
{
    puts("HID INIT");
    QUEUE_INIT(hid_queue);
}

//////////////////////////////////////////////////////////////////////

void hid_tick()
{
}

//////////////////////////////////////////////////////////////////////

void hid_update()
{
    // send key on/off to usb hid if there are some waiting to be sent
    if(!QUEUE_IS_EMPTY(hid_queue) && usb_is_endpoint_idle(endpoint_1) && usb_is_endpoint_idle(endpoint_2)) {
        uint8 p;
        QUEUE_POP(hid_queue, p);
        hid_process_event(p);
    }
}

//////////////////////////////////////////////////////////////////////

void hid_rotate(int8 direction)
{
    if(QUEUE_SPACE(hid_queue) >= 2) {

        if((hid_config.flags & cf_reverse_rotation) != 0) {
            direction = -direction;
        }

        switch(direction) {

        case ROTATE_CW:
            if((hid_config.flags & cf_led_flash_on_cw) != 0) {
                led_flash();
            }
            QUEUE_PUSH(hid_queue, (uint8)(event_clockwise | event_keydown));
            QUEUE_PUSH(hid_queue, (uint8)(event_clockwise | event_keyup));
            break;

        case ROTATE_CCW:
            if((hid_config.flags & cf_led_flash_on_ccw) != 0) {
                led_flash();
            }
            QUEUE_PUSH(hid_queue, (uint8)(event_counterclockwise | event_keydown));
            QUEUE_PUSH(hid_queue, (uint8)(event_counterclockwise | event_keyup));
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////

void hid_press()
{
    if(!QUEUE_IS_FULL(hid_queue)) {

        if((hid_config.flags & cf_led_flash_on_press) != 0) {
            led_flash();
        }

        QUEUE_PUSH(hid_queue, (uint8)(event_press | event_keydown));
    }
}

//////////////////////////////////////////////////////////////////////

void hid_release()
{
    if(!QUEUE_IS_FULL(hid_queue)) {

        if((hid_config.flags & cf_led_flash_on_release) != 0) {
            led_flash();
        }

        QUEUE_PUSH(hid_queue, (uint8)(event_press | event_keyup));
    }
}

//////////////////////////////////////////////////////////////////////

void hid_usb_receive(uint8 length)
{
    (void)length;

    printf("Custom HID: %d (got %d bytes)\n", usb_endpoint_3_rx_buffer[0], length);

    switch(usb_endpoint_3_rx_buffer[0]) {

    case hcc_get_config:
        if(usb_is_endpoint_idle(endpoint_3)) {
            usb_endpoint_3_tx_buffer[0] = hcc_here_is_config;
            memcpy(usb_endpoint_3_tx_buffer + 1, &hid_config, sizeof(hid_config));
            usb_send(endpoint_3, 32);
        }
        break;

    case hcc_set_config:
        memcpy(&hid_config, usb_endpoint_3_rx_buffer + 1, sizeof(hid_config));
        usb_endpoint_3_tx_buffer[0] = hcc_set_config_ack;
        usb_endpoint_3_tx_buffer[1] = 0;
        if(save_config()) {
            usb_endpoint_3_tx_buffer[1] = 1;
        }
        usb_send(endpoint_3, 32);
        break;

    case hcc_get_firmware_version:
        if(usb_is_endpoint_idle(endpoint_3)) {
            usb_endpoint_3_tx_buffer[0] = hcc_here_is_firmware_version;
            usb_endpoint_3_tx_buffer[1] = (uint8)(FIRMWARE_VERSION >> 0);
            usb_endpoint_3_tx_buffer[2] = (uint8)(FIRMWARE_VERSION >> 8);
            usb_endpoint_3_tx_buffer[3] = (uint8)(FIRMWARE_VERSION >> 16);
            usb_endpoint_3_tx_buffer[4] = (uint8)(FIRMWARE_VERSION >> 24);
            usb_send(endpoint_3, 32);
        }
        break;

    case hcc_flash_led:
        led_flash();
        if(usb_is_endpoint_idle(endpoint_3)) {
            usb_endpoint_3_tx_buffer[0] = hcc_led_flashed;
            usb_send(endpoint_3, 32);
        }
        break;

    case hcc_goto_bootloader:
        goto_bootloader();
        break;
    }
}

//////////////////////////////////////////////////////////////////////

__code const process_t hid_process = { .process_name = "HID",
                                       .on_init = hid_init,
                                       .on_tick = hid_tick,
                                       .on_update = hid_update,
                                       .on_rotate = hid_rotate,
                                       .on_press = hid_press,
                                       .on_release = hid_release,
                                       .on_usb_receive = { NULL, NULL, hid_usb_receive, NULL } };

__code const process_t *current_process = &hid_process;
