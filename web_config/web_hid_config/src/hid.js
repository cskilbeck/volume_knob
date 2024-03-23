//////////////////////////////////////////////////////////////////////

import { ref } from 'vue'
import keys from './hid_keys.js'

let hid_devices = ref({});

let scanned = ref({});

//////////////////////////////////////////////////////////////////////
// next midi device index

const CONFIG_VERSION = 0x82;

//////////////////////////////////////////////////////////////////////

const hid_custom_command = {
    hcc_get_config: 1,
    hcc_get_firmware_version: 2,
    hcc_flash_led: 3,
    hcc_set_config: 4,
    hcc_goto_bootloader: 5
};

//////////////////////////////////////////////////////////////////////

const hid_custom_response = {
    hcc_here_is_config: 1,
    hcc_here_is_firmware_version: 2,
    hcc_led_flashed: 3,
    hcc_set_config_ack: 4
};

//////////////////////////////////////////////////////////////////////
// config flags

const flags = {

    cf_led_flash_on_cw: 0x01,
    cf_led_flash_on_ccw: 0x02,
    cf_led_flash_on_press: 0x04,
    cf_led_flash_on_release: 0x08,
    cf_reverse_rotation: 0x10
};

//////////////////////////////////////////////////////////////////////

let default_flags = flags.cf_led_flash_on_cw | flags.cf_led_flash_on_ccw | flags.cf_led_flash_on_press;

let default_config = {

    config_version: CONFIG_VERSION,

    // what keys to send for rotation
    key_clockwise: 0xE9, //keys.consumer_control_keys['Volume Up'],
    key_counterclockwise: 0xEA, //keys.consumer_control_keys['Volume Down'],

    // key to send when pressed
    key_press: 0xE2, //keys.consumer_control_keys['Mute'],

    // modifiers
    mod_clockwise: 0,
    mod_counterclockwise: 0,
    mod_press: 0,

    // flags
    flags: default_flags
};

//////////////////////////////////////////////////////////////////////
// this is for marshalling the config struct to/from a byte array
// ugh, have to keep this in sync with firmware/config.h
// can't parse it directly because can't read or include files
// a tool to convert the C struct into this config_map would work but... no

// this assumes fields are laid out in order with no padding!!!

let config_map = [
    ["uint8", "config_version"],

    ["uint16", "key_clockwise"],
    ["uint16", "key_counterclockwise"],
    ["uint16", "key_press"],

    ["uint8", "mod_clockwise"],
    ["uint8", "mod_counterclockwise"],
    ["uint8", "mod_press"],

    ["uint16", "flags"]
];

// similar source code synchronization problem with the flags

//////////////////////////////////////////////////////////////////////
// this is super nasty - marshal/unmarshal from bytes to config struct

function config_from_bytes(bytes, offset) {

    let config = default_config;

    if (bytes[offset] != CONFIG_VERSION) {
        return default_config;
    }

    if (bytes.BYTES_PER_ELEMENT != 1) {
        return default_config;
    }

    let field_offset = 0;
    for (const field of config_map) {
        const field_type = field[0];
        const field_name = field[1];
        let field_size = 0;
        switch (field_type) {
            case 'uint8':
                field_size = 1;
                break;
            case 'uint16':
                field_size = 2;
                break;
        }
        if (field_size == 0) {
            console.log("ERROR: bad field size/offset array for unmarshalling config");
            return default_config;
        }
        let value = 0;
        for (let i = 0; i < field_size; ++i) {
            value |= bytes[field_offset + offset] << (i * 8);
            field_offset += 1;
        }
        config[field_name] = value;
    }
    return config;
}

//////////////////////////////////////////////////////////////////////

function bytes_from_config(config) {

    let bytes = [];

    for (const field of config_map) {
        const field_type = field[0];
        const field_name = field[1];
        let field_size = 0;
        switch (field_type) {
            case 'uint8':
                field_size = 1;
                break;
            case 'uint16':
                field_size = 2;
                break;
        }
        if (field_size == 0) {
            console.log("ERROR: bad field size/offset array for marshalling config");
            return null;
        }
        let value = config[field_name];
        for (let i = 0; i < field_size; ++i) {
            bytes.push(value & 0xff);
            value >>= 8;
        }
    }
    return new Uint8Array(bytes);
}

//////////////////////////////////////////////////////////////////////

function device_from_name(name) {
    if (hid_devices.value[name]) {
        return hid_devices.value[name];
    }
    return null;
}

//////////////////////////////////////////////////////////////////////

function device_from_hid_device(hid_device) {
    return device_from_name(hid_device.productName);
}

//////////////////////////////////////////////////////////////////////

async function send(device, data) {

    try {
        await device.hid_device.sendReport(0, new Uint8Array(data)).then(
            () => {
                //console.log(`Sent: [${data}]`);
            },
            (err) => {
                console.log(`Error sending report: ${err}`);
            });
    } catch (err) {
        console.log(err);
    }
}

//////////////////////////////////////////////////////////////////////

async function flash_device_led(device) {

    await send(device, [hid_custom_command.hcc_flash_led]);
}

//////////////////////////////////////////////////////////////////////

async function get_firmware_version(device) {

    await send(device, [hid_custom_command.hcc_get_firmware_version]);
}

//////////////////////////////////////////////////////////////////////

async function get_config(device) {

    await send(device, [hid_custom_command.hcc_get_config]);
}

//////////////////////////////////////////////////////////////////////

async function goto_firmware_update_mode(device) {

    await send(device, [hid_custom_command.hcc_goto_bootloader]);
}

//////////////////////////////////////////////////////////////////////

async function set_config(device) {

    let cur_config = bytes_from_config(device.config);
    let msg = new Uint8Array(cur_config.length + 1);
    msg[0] = hid_custom_command.hcc_set_config;
    for (let i = 0; i < cur_config.length; ++i) {
        msg[i + 1] = cur_config[i];
    }
    await send(device, msg);
}

//////////////////////////////////////////////////////////////////////

function on_hid_input_report(e) {

    let device = device_from_hid_device(e.device);

    if (device != null) {

        let data = new Uint8Array(e.data.buffer);
        //console.log(`Got ${data[0]}`);

        switch (data[0]) {

            case hid_custom_response.hcc_here_is_firmware_version:
                let b0 = data[1] || 0;
                let b1 = data[2] || 0;
                let b2 = data[3] || 0;
                let b3 = data[4] || 0;
                device.firmware_version = b3 | (b2 << 7) | (b1 << 14) | (b0 << 21);
                device.firmware_version_str = `${b3}.${b2}.${b1}.${b0}`;
                console.log(`Firmware version ${device.firmware_version_str}`);
                break;

            case hid_custom_response.hcc_here_is_config:
                device.config = config_from_bytes(data, 1);
                if (device.on_config_loaded != null) {
                    device.on_config_loaded();
                }
                console.log("Got config", device.config);
                break;

            case hid_custom_response.hcc_set_config_ack:
                if (device.on_config_saved != null) {
                    device.on_config_saved();
                }
                break;
        }
    } else {
        console.log(`Unknown device ${e.device.productName}`);
    }
}

//////////////////////////////////////////////////////////////////////

async function init_device(d) {

    let device = hid_devices.value[d.productName];

    if (!device) {

        console.log(`New device ${d.productName}`);

        device = {
            firmware_version: 0x00000000,
            firmware_version_str: "0.0.0.0",
            hid_device: d,
            name: d.productName,
            config: {},
            on_config_loaded: null,
            on_config_saved: null
        };

        Object.assign(device.config, default_config);

        Object.defineProperty(device, 'active', {
            get() {
                return this.hid_device != null && this.hid_device.opened;
            }
        });

        d.addEventListener("inputreport", on_hid_input_report);

        hid_devices.value[d.productName] = device;
    }

    await get_config(device);
    await get_firmware_version(device);
}

//////////////////////////////////////////////////////////////////////

function on_connection_event(event) {

    let name = event.device.productName;
    if (hid_devices.value[name] && event.type == 'disconnect') {
        delete hid_devices.value[name];
    }
}

//////////////////////////////////////////////////////////////////////

function init_devices() {

    navigator.hid.removeEventListener("connect", on_connection_event);
    navigator.hid.removeEventListener("disconnect", on_connection_event);

    navigator.hid.addEventListener("connect", on_connection_event);
    navigator.hid.addEventListener("disconnect", on_connection_event);

    navigator.hid.requestDevice({
        filters: [{
            vendorId: 0x16D0,
            productId: 0x114B,
            usage: 1,
            usagePage: 0xFF00
        }]
    }).then(

        (devices) => {
            scanned.value.done = true;

            for (let d of devices) {
                if (d.opened) {
                    console.log(`${d.productName} already open...`);
                    init_device(d);
                } else {
                    d.open().then(() => {
                        console.log(`${d.productName} opened`);
                        init_device(d);
                    });
                }
            }
        });
}

//////////////////////////////////////////////////////////////////////

export default {
    hid_devices,
    default_config,
    scanned,
    flags,
    flash_device_led,
    goto_firmware_update_mode,
    get_config,
    set_config,
    init_devices
}
