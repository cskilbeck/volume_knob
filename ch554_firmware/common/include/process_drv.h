#pragma once

typedef void (*on_init_fn)(void);
typedef void (*on_tick_fn)(void);
typedef void (*on_update_fn)(void);
typedef void (*on_rotate_fn)(int8 direction);
typedef void (*on_press_fn)(void);
typedef void (*on_release_fn)(void);
typedef void (*on_usb_receive_fn)(uint8 length);

struct process_s
{
    char const *process_name;
    on_init_fn on_init;
    on_tick_fn on_tick;
    on_update_fn on_update;
    on_rotate_fn on_rotate;
    on_press_fn on_press;
    on_release_fn on_release;
    on_usb_receive_fn on_usb_receive[4];    // 4 endpoints
};

typedef struct process_s process_t;

extern __code const process_t *current_process;    // there can be only one process (midi or hid)
