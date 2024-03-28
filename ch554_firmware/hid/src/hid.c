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
    uint8 modifiers = 0;
    uint16 key = 0;

    switch(e & event_mask) {

    case event_clockwise:
        key = config.key_clockwise;
        modifiers = config.mod_clockwise;
        break;

    case event_counterclockwise:
        key = config.key_counterclockwise;
        modifiers = config.mod_counterclockwise;
        break;

    case event_press:
        key = config.key_press;
        modifiers = config.mod_press;
        break;
    }

    uint16 send_key = key & 0x7fff;
    uint16 send_modifiers = modifiers;

    if((e & event_keyup) != 0) {
        send_key = 0;
        send_modifiers = 0;
    }

    if(IS_MEDIA_KEY(key)) {

        consumer_control_hid_report *cc = (consumer_control_hid_report *)usb_endpoint_1_tx_buffer;
        cc->report_id = 0x02;
        cc->keycode = send_key;
        usb_send(endpoint_1, 3);

    } else {

        keyboard_hid_report *k = ((keyboard_hid_report *)usb_endpoint_1_tx_buffer);
        k->report_id = 0x01;
        k->modifiers = send_modifiers;
        k->pad = 0;
        k->key[0] = send_key;
        k->key[1] = 0;
        k->key[2] = 0;
        k->key[3] = 0;
        k->key[4] = 0;
        k->key[5] = 0;
        usb_send(endpoint_1, 9);
    }
}

//////////////////////////////////////////////////////////////////////

void hid_init()
{
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
    if(!QUEUE_IS_EMPTY(hid_queue) && usb_is_endpoint_idle(endpoint_1)) {
        uint8 p;
        QUEUE_POP(hid_queue, p);
        hid_process_event(p);
    }
}

//////////////////////////////////////////////////////////////////////

void hid_rotate(int8 direction)
{
    if(QUEUE_SPACE(hid_queue) >= 2) {

        if((config.flags & cf_reverse_rotation) != 0) {
            direction = -direction;
        }

        switch(direction) {

        case -1:
            if((config.flags & cf_led_flash_on_cw) != 0) {
                led_flash();
            }
            QUEUE_PUSH(hid_queue, (uint8)(event_clockwise | event_keydown));
            QUEUE_PUSH(hid_queue, (uint8)(event_clockwise | event_keyup));
            break;

        case 1:
            if((config.flags & cf_led_flash_on_ccw) != 0) {
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

        if((config.flags & cf_led_flash_on_press) != 0) {
            led_flash();
        }

        QUEUE_PUSH(hid_queue, (uint8)(event_press | event_keydown));
    }
}

//////////////////////////////////////////////////////////////////////

void hid_release()
{
    if(!QUEUE_IS_FULL(hid_queue)) {

        if((config.flags & cf_led_flash_on_release) != 0) {
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
            memcpy(usb_endpoint_3_tx_buffer + 1, &config, sizeof(config));
            usb_send(endpoint_3, 32);
        }
        break;

    case hcc_set_config:
        memcpy(&config, usb_endpoint_3_rx_buffer + 1, sizeof(config));
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
