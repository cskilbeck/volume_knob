//////////////////////////////////////////////////////////////////////

import { ref } from 'vue'
import keys from './hid_keys.js'

let hid_devices = ref([]);

let scanned = ref({});

//////////////////////////////////////////////////////////////////////
// next midi device index

let device_index = 0;

const CONFIG_VERSION = 0x80;

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

    // if key_release != 0 then don't send key up after key_press
    // and send key_release when released
    key_release: 0,

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
    ["uint16", "key_release"],
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
// convert some bytes to a hex string

function bytes_to_hex_string(data, len, separator) {
    if (separator == undefined) {
        separator = " ";
    }
    let sep = "";
    let str = "";
    if (len === undefined) {
        len = data.length;
    }
    let l = Math.min(len, data.length);
    for (let i = 0; i < l; ++i) {
        str += sep + data[i].toString(16).toUpperCase().padStart(2, '0');
        sep = separator;
    }
    return str;
}

//////////////////////////////////////////////////////////////////////

function device_from_index(index) {
    if (index < hid_devices.value.length) {
        return hid_devices.value[index];
    }
    console.log(`No device from index ${index}`);
    return null;
}

//////////////////////////////////////////////////////////////////////

async function send(index, data) {

    let device = device_from_index(index);
    if (device == null) {
        console.log(`SEND: no device for ${index}`);
        return;
    }

    console.log(`SEND: [${data}]`);

    try {
        await device.hid_device.sendReport(0, new Uint8Array(data)).then(
            () => {
                console.log(`Sent: [${data}]`);
            },
            (err) => {
                console.log(`Error sending report: ${err}`);
            });
    } catch (err) {
        console.log(err);
    }
}

//////////////////////////////////////////////////////////////////////

async function flash_device_led(index) {

    await send(index, [hid_custom_command.hcc_flash_led]);
}

//////////////////////////////////////////////////////////////////////

async function get_firmware_version(index) {

    await send(index, [hid_custom_command.hcc_get_firmware_version]);
}

//////////////////////////////////////////////////////////////////////

async function get_config(index) {

    await send(index, [hid_custom_command.hcc_get_config]);
}

//////////////////////////////////////////////////////////////////////

async function goto_firmware_update_mode(index) {
    await send(index, [hid_custom_command.hcc_goto_bootloader]);
}

//////////////////////////////////////////////////////////////////////

async function set_config(index) {
    let device = device_from_index(index);
    if (device == null) {
        console.log(`set_config: no device at index ${index}`);
        return;
    }
    let cur_config = bytes_from_config(device.config);
    let msg = new Uint8Array(cur_config.length + 1);
    msg[0] = hid_custom_command.hcc_set_config;
    for (let i = 0; i < cur_config.length; ++i) {
        msg[i + 1] = cur_config[i];
    }
    await send(index, msg);
}

//////////////////////////////////////////////////////////////////////

function on_hid_input_report(index, e) {

    let device = device_from_index(index);
    if (device != null) {
        let data = new Uint8Array(e.data.buffer);
        console.log(`Got ${data[0]}`);
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
        }
    } else {
        console.log(`No device at index ${index}`);
    }
}

//////////////////////////////////////////////////////////////////////

async function init_device(d) {

    for (let hid_device of hid_devices.value) {
        if (hid_device.name == d.productName) {
            console.log(`Already got ${d.productName}`);
            return;
        }
    }

    console.log(`New device ${d.productName}`);

    let device = {
        device_index: device_index,
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

    hid_devices.value[device_index] = device;

    console.log(`add device ${device_index}`);
    d.addEventListener("inputreport", (event) => {
        on_hid_input_report(device.device_index, event);
    });

    await get_config(device.device_index);
    await get_firmware_version(device.device_index);

    device_index += 1;
}

//////////////////////////////////////////////////////////////////////

function init_devices() {
    navigator.hid.requestDevice({
        filters: [{
            vendorId: 0x16D0,
            productId: 0x114B,
            usage: 1,
            usagePage: 0xFF00
        }]
    }).then(

        (devices) => {
            scanned.done = true;

            for (let d of devices) {
                console.log("FOUND", d.productName, d.opened);
                if (d.opened) {
                    init_device(d);
                } else {
                    d.open().then(() => {
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
