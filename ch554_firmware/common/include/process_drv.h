#pragma once

typedef void (*on_init_fn)(void);
typedef void (*on_tick_fn)(void);
typedef void (*on_update_fn)(void);
typedef void (*on_usb_receive_fn)(unsigned char endpoint, unsigned char length);

struct process_s
{
    char const *product_name;

    on_init_fn on_init;
    on_tick_fn on_tick;
    on_update_fn on_update;
    on_usb_receive_fn on_usb_receive;
};

typedef struct process_s process_t;

extern process_t *current_process;    // there can be only one process (midi or hid)
