//////////////////////////////////////////////////////////////////////
// web midi stuff

import { ref, toRaw } from 'vue'

const CONFIG_LEN = 26;

const CONFIG_VERSION = 0x07

const MIDI_MANUFACTURER_ID = 0x36;    // Cheetah Marketing, defunct?

const MIDI_FAMILY_CODE_LOW = 0x44;   // 0x5544
const MIDI_FAMILY_CODE_HIGH = 0x55;

const MIDI_MODEL_NUMBER_LOW = 0x22;   // 0x3322
const MIDI_MODEL_NUMBER_HIGH = 0x33;

//////////////////////////////////////////////////////////////////////
// main web midi object

let midi = null;

//////////////////////////////////////////////////////////////////////
// all the midi devices and configs which have responded to device id requests
// position in the array is device_index

let midi_devices = ref([]);

// if a scan has been done (for show/hide device count)

let scanned = ref({});

//////////////////////////////////////////////////////////////////////
// next midi device index

let device_index = 0;

//////////////////////////////////////////////////////////////////////

const sysex_request_device_id = 0x01;
const sysex_request_toggle_led = 0x02;
const sysex_request_get_flash = 0x03;
const sysex_request_set_flash = 0x04;
const sysex_request_bootloader = 0x05;

//const sysex_response_unused_01 = 0x01;
const sysex_response_device_id = 0x02;
const sysex_response_get_flash = 0x03;
const sysex_response_set_flash_ack = 0x04;

//////////////////////////////////////////////////////////////////////
// config flags

const flags = {

    // rotate modifies both CC MSB and LSB
    cf_rotate_extended: 0x0001,

    // rotate sends relative changes
    cf_rotate_relative: 0x0002,

    // led on means off and vice versa
    cf_led_invert: 0x0004,

    // flash when knob is rotated
    cf_led_flash_on_rot: 0x0008,

    // flash when rotation limit is hit (if not cf_rotate_relative)
    cf_led_flash_on_limit: 0x0010,

    // btn sets value1/value2 based on state of button rather than toggling between them
    cf_btn_momentary: 0x0020,

    // button modifies both CC MSB and LSB
    cf_btn_extended: 0x0040,

    // flash led when button is clicked
    cf_led_flash_on_click: 0x0080,

    // flash led when button is released
    cf_led_flash_on_release: 0x0100,

    // led tracks state of button
    cf_led_track_button_toggle: 0x0200,

    // two bits for acceleration (so 4 options: off; low; med; high)
    cf_acceleration_lsb: 0x0400,
    cf_acceleration_msb: 0x0800,

    // current button toggle state
    cf_toggle: 0x1000,

    // button's second value ('released') tracks rotation value (for e.g. mute/unmute)
    cf_button_tracks_rotation: 0x2000
};

//////////////////////////////////////////////////////////////////////

let default_flags = flags.cf_led_flash_on_limit | flags.cf_led_flash_on_click | flags.cf_acceleration_lsb;

let default_config = {
    config_version: CONFIG_VERSION, // config struct version - must be 1st byte!
    rot_control_msb: 7,             // Control Change index MSB,LSB for knob
    rot_control_lsb: 39,            // Control Change index MSB,LSB for knob
    btn_control_msb: 3,             // Control Change index MSB,LSB for button
    btn_control_lsb: 35,            // Control Change index MSB,LSB for button
    btn_value_a_14: 0x3fff,         // 1st,2nd button values or pressed/released values if cf_btn_momentary (14 bit mode)
    btn_value_b_14: 0,              // 1st,2nd button values or pressed/released values if cf_btn_momentary (14 bit mode)
    btn_value_a_7: 0x7f,            // 1st,2nd button values or pressed/released values if cf_btn_momentary (7 bit mode)
    btn_value_b_7: 0,               // 1st,2nd button values or pressed/released values if cf_btn_momentary (7 bit mode)
    channels: 0,                    // rotate channel in low nibble, button in high nibble
    rot_zero_point: 0x40,           // Zero point in relative mode (forced 7 bit mode)
    rot_delta_14: 1,                // How much to change by (14 bit mode)
    rot_delta_7: 1,                 // How much to change by(7 bit mode)
    rot_current_value_14: 0,        // current value (in absolute mode) (14 bit mode)
    rot_current_value_7: 0,         // current value (in absolute mode) (7 bit mode)
    flags: default_flags            // flags, see enum above
};

//////////////////////////////////////////////////////////////////////
// this is for marshalling the config struct to/from a byte array
// ugh, have to keep this in sync with firmware/config.h
// can't parse it directly because can't read or include files
// a tool to convert the C struct into this config_map would work but... no

// this assumes fields are laid out in order with no padding!!!

let config_map = [
    ["uint8", "config_version"],
    ["uint8", "rot_control_msb"],
    ["uint8", "rot_control_lsb"],
    ["uint8", "btn_control_msb"],
    ["uint8", "btn_control_lsb"],
    ["uint16", "btn_value_a_14"],
    ["uint16", "btn_value_b_14"],
    ["uint8", "btn_value_a_7"],
    ["uint8", "btn_value_b_7"],
    ["uint8", "channels"],
    ["uint8", "rot_zero_point"],
    ["uint16", "rot_delta_14"],
    ["uint8", "rot_delta_7"],
    ["uint16", "rot_current_value_14"],
    ["uint8", "rot_current_value_7"],
    ["uint16", "flags"],
    ["uint16", "firmware_version"]
];

// similar source code synchronization problem with the flags

//////////////////////////////////////////////////////////////////////
// this is super nasty - marshal/unmarshal from bytes to config struct

function config_from_bytes(bytes) {

    let config = default_config;

    if (bytes[0] != CONFIG_VERSION) {
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
            value |= bytes[field_offset] << (i * 8);
            field_offset += 1;
        }
        config[field_name] = value;
    }
    return config;
}

//////////////////////////////////////////////////////////////////////

function bytes_from_config(config) {

    let bytes = new Uint8Array(CONFIG_LEN);

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
            console.log("ERROR: bad field size/offset array for marshalling config");
            return null;
        }
        let value = config[field_name];
        for (let i = 0; i < field_size; ++i) {
            bytes[field_offset] = value & 0xff;
            value >>= 8;
            field_offset += 1;
        }
    }
    return bytes;
}

//////////////////////////////////////////////////////////////////////
// expand some bytes into an array of 7 bit values

function bytes_to_bits7(src_data, offset, src_len) {

    if (src_data.BYTES_PER_ELEMENT != 1) {
        return null;
    }

    let required_bytes = (((src_len * 8) + 7) / 7) | 0;
    let dest = new Uint8Array(required_bytes);

    let bits_available = 0;
    let cur_src = 0;
    let dst_offset = 0;
    let bits_remaining = src_len * 8;

    while (dst_offset < required_bytes) {

        if (bits_available < 7) {
            cur_src = (cur_src << 8) | src_data[offset];
            offset += 1;
            bits_available += 8;
        }

        while (bits_available >= 7) {
            bits_available -= 7;
            dest[dst_offset] = (cur_src >> bits_available) & 0x7f;
            dst_offset += 1;
            bits_remaining -= 7;
        }

        if (bits_remaining < 7) {
            dest[dst_offset] = (cur_src << (7 - bits_remaining)) & 0x7f;
            dst_offset += 1;
        }
    }
    return dest;
}

//////////////////////////////////////////////////////////////////////
// convert some 7 bit values to bytes

function bits7_to_bytes(src_data, offset, dst_len) {

    if (src_data.BYTES_PER_ELEMENT != 1) {
        return;
    }

    let dest = new Uint8Array(dst_len);

    let bits_available = 0;
    let cur_src = 0;
    let dst_offset = 0;

    while (dst_offset < dst_len) {

        if (bits_available < 8) {
            cur_src = (cur_src << 7) | src_data[offset];
            offset += 1;
            bits_available += 7;
        }

        while (bits_available >= 8) {
            bits_available -= 8;
            dest[dst_offset] = (cur_src >> bits_available) & 0xff;
            dst_offset += 1;
        }
    }
    return dest;
}

//////////////////////////////////////////////////////////////////////

function flash_device_led(index) {
    const device = midi_devices.value[index];
    if (device === undefined) {
        console.log(`Can't find device ${index}`);
    } else {
        console.log(`Flash led for device ${device.serial_number.toString(16).toUpperCase()}`);
        send_midi(device, [0xF0, 0x7E, 0x00, 0x06, sysex_request_toggle_led, 0xF7]);
    }
}

//////////////////////////////////////////////////////////////////////

function flash_mode(index) {
    const device = midi_devices.value[index];
    if (device === undefined) {
        console.log(`Can't find device ${index}`);
    } else {
        console.log(`Enter flash mode for device ${device.serial_number.toString(16).toUpperCase()}`);
        send_midi(device, [0xF0, 0x7E, 0x00, 0x06, sysex_request_bootloader, 0xF7]);
    }
}

//////////////////////////////////////////////////////////////////////

function on_device_id_response(input_port, data) {

    let reply_index = data[2];
    let device = midi_devices.value[reply_index];
    if (device == undefined) {
        console.log(`Midi from unknown device!`);
        return;
    }

    console.log(`New device on ${device.output.id} / ${input_port.id} ?`);

    // check MANUFACTURER, FAMILY, MODEL
    if (!(data[5] == MIDI_MANUFACTURER_ID &&
        data[6] == MIDI_FAMILY_CODE_LOW &&
        data[7] == MIDI_FAMILY_CODE_HIGH &&
        data[8] == MIDI_MODEL_NUMBER_LOW &&
        data[9] == MIDI_MODEL_NUMBER_HIGH)) {

        console.log(`Unrecognized device, ignoring...`);
        return;
    }

    // get the serial number from the device id response

    let b0 = data[10] || 0;
    let b1 = data[11] || 0;
    let b2 = data[12] || 0;
    let b3 = data[13] || 0;

    device.serial_number = b3 | (b2 << 7) | (b1 << 14) | (b0 << 21);
    device.serial_str = device.serial_number.toString(16).toUpperCase();

    // add a new device to the array of midi_devices

    console.log(`Found device ${device.name}, serial # ${device.serial_str}, ${midi_devices.value.length} device(s) so far...`);

    device.input = input_port;

    // get the config
    read_flash(reply_index);
}

//////////////////////////////////////////////////////////////////////
// send a request for the flash contents

function read_flash(index) {
    const device = midi_devices.value[index];
    if (device === undefined) {
        console.log(`read_flash: No such device ${index}`);
        return;
    }
    send_midi(device, [0xF0, 0x7E, 0x00, 0x06, sysex_request_get_flash, 0xF7]);
}

//////////////////////////////////////////////////////////////////////
// send new flash contents

function write_flash(index) {
    const device = midi_devices.value[index];
    if (device === undefined) {
        console.log(`write_flash: No such device ${index}`);
        return;
    }
    let data_7bits = bytes_to_bits7(bytes_from_config(device.config), 0, CONFIG_LEN);
    if (data_7bits !== null) {
        send_midi(device, [0xF0, 0x7E, 0x00, 0x06, sysex_request_set_flash].concat(Array.from(data_7bits)).concat([0xF7]));
    } else {
        // report error
    }
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
// send some data to a midi device

function send_midi(midi_device, data) {
    // console.log(`SEND: ${bytes_to_hex_string(data, data.length, " ")}`);
    midi_device.output.send(data);
}

//////////////////////////////////////////////////////////////////////

function init_devices() {

    console.log(`init_devices`);

    midi_devices.value = [];
    device_index = 0;

    scanned.done = true;

    console.log(`${midi.inputs.size} inputs, ${midi.outputs.size} outputs`);

    // initial setup of callbacks for midi messages

    for (const input of midi.inputs.values()) {
        console.log(`Found ${input.name} at ${input.id}`);
        input.addEventListener("midimessage", on_midi_message);
    }

    for (const output of midi.outputs.values()) {

        console.log(`Found ${output.name} at ${output.id}`);

        let device = {
            device_index: device_index,
            serial_number: 0,
            serial_str: "#######",
            input: null,            // inputs get assigned when replies come back
            output: output,
            name: output.name,
            config: {}
        };

        Object.assign(device.config, default_config);

        Object.defineProperty(device, 'active', {
            get() {
                return this.input != null && this.input.state == 'connected';
            }
        });

        midi_devices.value[device_index] = device;

        device_index += 1;
    }
    console.log(`init devices scanned ${device_index} devices`);
}

//////////////////////////////////////////////////////////////////////

function connect_device(d) {
    d.output.send([0xF0, 0x7E, d.device_index & 0x7f, 0x06, sysex_request_device_id, 0xF7]);
}

//////////////////////////////////////////////////////////////////////
// port.close() doesn't seem to work so this is kind of moot

function toggle_device_connection(device_index) {
    console.log(`Toggle device ${device_index} connection`);
    let d = midi_devices.value[device_index];
    if (d != null) {
        if (d.active) {
            console.log(`Closing ${d.input.name}:${d.input.state}`);
            d.input.close();
            d.output.close();
            console.log("Closed");
        } else {
            connect_device(d);
        }
    }
}

//////////////////////////////////////////////////////////////////////

function get_sysex_device_index(data) {

    if (data[0] == 0xF0 &&
        data[1] == 0x7E &&
        data[3] == 0x07 &&
        data[data.length - 1] == 0xF7) {

        return data[2];
    }
    return undefined;
}

//////////////////////////////////////////////////////////////////////

function on_midi_message(event) {

    let input_port = event.target;

    // console.log(event);

    const data = event.data;

    // console.log(`RECV: ${bytes_to_hex_string(data, data.length, " ")}`);

    let midi_status = data[0] & 0xf0;

    switch (midi_status) {

        // B0 is control change
        case 0xB0: {

            if (on_control_change_callback != null) {
                on_control_change_callback(data[0] & 0xf, data[1], data[2]);
            }

        } break;

        // F0 is sysex
        case 0xF0: {

            let device_index = get_sysex_device_index(data);

            if (device_index !== undefined) {

                switch (data[4]) {

                    // device ID response
                    case sysex_response_device_id: {
                        on_device_id_response(input_port, data);
                    } break;

                    // read flash memory response
                    case sysex_response_get_flash: {
                        let d = midi_devices.value[device_index];
                        if (d !== undefined) {
                            let flash_data = bits7_to_bytes(data, 5, CONFIG_LEN);
                            d.config = config_from_bytes(flash_data);
                            let s = bytes_to_hex_string(flash_data, CONFIG_LEN);
                            console.log(`Memory for device ${d.serial_str}: ${s}`);

                            if (on_config_loaded_callback != null) {
                                on_config_loaded_callback(d);
                            }
                        }
                    } break;

                    // write flash memory ACK
                    case sysex_response_set_flash_ack: {
                        let d = midi_devices.value[device_index];
                        if (d !== undefined) {
                            console.log(`Device ${d.serial_str} wrote flash data`);
                            if (on_config_saved_callback != null) {
                                on_config_saved_callback(d);
                            }
                        }
                    } break;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////

function get_config_json(device_index) {
    let c = {
        error: "No such device"
    };
    let d = midi_devices.value[device_index];
    if (d != undefined) {
        c = Object.assign({}, d.config);
        delete c.value;
    }
    return JSON.stringify(c, null, 4);
}

//////////////////////////////////////////////////////////////////////

let on_config_loaded_callback = null;

function on_config_loaded(callback) {
    on_config_loaded_callback = callback;
}

//////////////////////////////////////////////////////////////////////

let on_config_saved_callback = null;

function on_config_saved(callback) {
    on_config_saved_callback = callback;
}

//////////////////////////////////////////////////////////////////////

let on_control_change_callback = null;

function on_control_change(callback) {
    on_control_change_callback = callback;
}

//////////////////////////////////////////////////////////////////////

function on_midi_startup(midi_obj) {

    midi = midi_obj;

    midi.addEventListener('statechange', function (event) {

        console.log(`statechange: ${event.port.name} (${event.port.id}): ${event.port.state}`);

        if (event.port.state == 'disconnected') {
            for (let device of midi_devices.value) {
                if (device.input && device.input.id == event.port.id) {
                    device.input = null;
                } else if (device.output && device.output.id == event.port.id) {
                    device.output = null;
                }
            }
        } else if (event.port.state == 'connected') {
            for (let device of midi_devices.value) {
            }
        }
    });
}

//////////////////////////////////////////////////////////////////////

export default {
    midi,
    midi_devices,
    scanned,
    on_midi_startup,
    init_devices,
    connect_device,
    toggle_device_connection,
    on_config_loaded,
    on_config_saved,
    on_control_change,
    flash_device_led,
    flash_mode,
    read_flash,
    write_flash,
    get_config_json,
    flags
}
