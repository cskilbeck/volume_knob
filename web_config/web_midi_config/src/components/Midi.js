//////////////////////////////////////////////////////////////////////
// web midi stuff

import { ref } from 'vue'

const FLASH_MAX_LEN = 26;

//////////////////////////////////////////////////////////////////////
// main web midi object

let midi = null;

//////////////////////////////////////////////////////////////////////
// all the midi devices and configs which have responded to device id requests
// position in the array is device_index

// configs in a separate array for reactivity purposes

let midi_devices = ref([]);

let configs = ref([]);

//////////////////////////////////////////////////////////////////////
// output ports which have been sent a device id request
// position in the array is device_index

let output_ports = [];

//////////////////////////////////////////////////////////////////////
// next midi device index

let device_index = 0;

//////////////////////////////////////////////////////////////////////

let default_config = {
    cc_msb: 3,
    cc_lsb: 35,
    zero_point: 64,
    delta: 1,
    extended: true,
    relative: false,
    acceleration: 0
};

/*

// 16 flags available

typedef enum flags
{
    extended = 1,
    relative = 2,
    accel_0 = 4,
    accel_1 = 8,
    momentary = 16

} flags_t;

// sizeof(config_t) should be 26 bytes

typedef struct config {
    uint8 rot_control_low;
    uint8 rot_control_high;
    uint16 rot_zero_point;
    uint16 rot_current_value;
    uint16 rot_delta;

    uint8 btn_cc_low;
    uint8 btn_cc_high;
    uint8 btn_pressed_value;
    uint8 btn_released_value;
    uint16 cf_flags;

} config_t;

*/

//////////////////////////////////////////////////////////////////////

function config_from_bytes(bytes) {
    let config = default_config;
    config.cc_msb = bytes[0] & 0x7f;
    config.cc_lsb = bytes[1] & 0x7f;
    config.zero_point = bytes[2] & 0xff;
    config.zero_point |= (bytes[3] & 0x3f) << 8;
    config.delta = bytes[4] & 0xff;
    config.delta |= (bytes[5] & 0x3f) << 8;
    let flags = (bytes[6] & 0xff) | ((bytes[7] & 0xff) << 8);
    config.extended = (flags & 1) != 0;
    config.relative = (flags & 2) != 0;
    config.acceleration = (flags >> 2) & 3;
    return config;
}

//////////////////////////////////////////////////////////////////////

function bytes_from_config(config) {
    let bytes = new Array(26).fill(0);
    bytes[0] = config.cc_msb & 0x7f;
    bytes[1] = config.cc_lsb & 0x7f;
    bytes[2] = config.zero_point & 0xff;
    bytes[3] = (config.zero_point >> 8) & 0x3f;
    bytes[4] = config.delta & 0xff;
    bytes[5] = (config.delta >> 8) & 0x3f;
    let flags = 0;
    flags |= config.extended ? 1 : 0;
    flags |= config.relative ? 2 : 0;
    flags |= (config.acceleration & 3) << 2;
    bytes[6] = flags & 0xff;
    bytes[7] = (flags >> 8) & 0xff;
    return bytes;
}

//////////////////////////////////////////////////////////////////////
// expand some bytes into an array of 7 bit values

function bytes_to_bits7(src_data, offset, len, dest) {

    let bits_available = 0;
    let cur_src = 0;
    let bits_remaining = len * 8;

    while (true) {

        if (bits_available < 7) {
            cur_src = (cur_src << 8) | src_data[offset++];
            bits_available += 8;
        }

        while (bits_available >= 7) {
            bits_available -= 7;
            dest.push((cur_src >> bits_available) & 0x7f);
            bits_remaining -= 7;
        }

        if (bits_remaining < 7) {
            dest.push((cur_src << (7 - bits_remaining)) & 0x7f);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// convert some 7 bit values to bytes

function bits7_to_bytes(src_data, offset, len, dest) {

    let bits_available = 0;
    let cur_src = 0;

    while (dest.length < len) {

        if (bits_available < 8) {
            cur_src = (cur_src << 7) | src_data[offset++];
            bits_available += 7;
        }

        while (bits_available >= 8) {
            bits_available -= 8;
            dest.push((cur_src >> bits_available) & 0xff);
        }
    }
}

//////////////////////////////////////////////////////////////////////

function toggle_device_led(index) {
    const device = midi_devices.value[index];
    if (device === undefined) {
        console.log(`Can't find device ${index}`);
    } else {
        console.log(`Toggle led for device ${device.serial_number.toString(16).toUpperCase()}`);
        send_midi(device, [0xF0, 0x7E, 0x00, 0x06, 0x02, 0xF7]);
    }
}

//////////////////////////////////////////////////////////////////////

const MIDI_MANUFACTURER_ID = 0x36;    // Cheetah Marketing, defunct?

const MIDI_FAMILY_CODE_LOW = 0x44;   // 0x5544
const MIDI_FAMILY_CODE_HIGH = 0x55;

const MIDI_MODEL_NUMBER_LOW = 0x22;   // 0x3322
const MIDI_MODEL_NUMBER_HIGH = 0x33;

//////////////////////////////////////////////////////////////////////

function handle_new_device(input_port, data) {

    let reply_index = data[2];
    let output_port = output_ports[reply_index];
    if (output_port == undefined) {
        console.log(`Midi from unknown device: ${device.name}`);
        return;
    }

    console.log(`New device on ${output_port.id} / ${input_port.id} ?`);

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

    let serial_number = b3 | (b2 << 7) | (b1 << 14) | (b0 << 21);
    let serial_str = serial_number.toString(16).toUpperCase()

    // add a new device to the array of midi_devices

    let device = {
        device_index: reply_index,
        serial_number: serial_number,
        serial_str: serial_str,
        input: input_port,
        output: output_port,
        name: input_port.name,
        config: ref({})
    };
    device.config.value = default_config;
    midi_devices.value[reply_index] = device;

    console.log(`Found device ${device.name}, serial # ${device.serial_str}, ${midi_devices.value.length} device(s) so far...`);

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
    send_midi(device, [0xF0, 0x7E, 0x00, 0x06, 0x03, 0xF7]);
}

//////////////////////////////////////////////////////////////////////
// send new flash contents

function write_flash(index) {
    const device = midi_devices.value[index];
    if (device === undefined) {
        console.log(`write_flash: No such device ${index}`);
        return;
    }
    let data_7bits = [];
    bytes_to_bits7(bytes_from_config(device.config.value), 0, FLASH_MAX_LEN, data_7bits);
    send_midi(device, [0xF0, 0x7E, 0x00, 0x06, 0x04].concat(data_7bits).concat([0xF7]));
}

//////////////////////////////////////////////////////////////////////
// Convert a hex string to a byte array

function hex_to_bytes(hex) {
    let bytes = [];
    for (let c = 0; c < hex.length; c += 2) {
        bytes.push(parseInt(hex.substr(c, 2), 16));
    }
    while (bytes.length < FLASH_MAX_LEN) {
        bytes.push(0xFF);
    }
    return bytes;
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
    console.log(`SEND: ${bytes_to_hex_string(data, data.length, " ")}`);
    midi_device.output.send(data);
}

//////////////////////////////////////////////////////////////////////

function init_devices() {

    console.log(`init_devices`);

    midi_devices.value = [];
    output_ports = [];
    device_index = 0;

    console.log(`${midi.inputs.size} inputs, ${midi.outputs.size} outputs`);

    for (const input of midi.inputs.values()) {
        console.log(`Found ${input.name} at ${input.id}`);
        input.onmidimessage = function (event) {
            on_midi_message(input, event);
        };
    }

    for (const output of midi.outputs.values()) {
        console.log(`Found ${output.name} at ${output.id}`);

        // for looking up the device when the response arrives
        output_ports[device_index] = output;
        output.send([0xF0, 0x7E, device_index & 0x7f, 0x06, 0x01, 0xF7]);
        device_index += 1;
    }
    console.log(`init devices scanned ${device_index} devices`);
}

//////////////////////////////////////////////////////////////////////

function is_null_or_undefined(x) {
    return x === null || x === undefined;
}

//////////////////////////////////////////////////////////////////////

function array_compare(a, b) {
    return a.length === b.length &&
        a.every((element, index) => element === b[index]);
}

//////////////////////////////////////////////////////////////////////

function get_sysex_device_index(data) {
    if (data[0] == 0xF0 && data[1] == 0x7E && data[3] == 0x07 && data[data.length - 1] == 0xF7) {
        return data[2];
    }
    return undefined;
}

//////////////////////////////////////////////////////////////////////

function on_midi_message(input_port, event) {

    const data = event.data;

    console.log(`RECV: ${bytes_to_hex_string(data, data.length, " ")}`);

    let device_index = get_sysex_device_index(data);

    if (device_index != undefined) {

        switch (data[4]) {

            // device ID response
            case 0x02: {
                handle_new_device(input_port, data);
            } break;

            // read flash memory response
            case 0x3: {
                let d = midi_devices.value[device_index];
                if (d !== undefined) {
                    let flash_data = [];
                    bits7_to_bytes(data, 5, FLASH_MAX_LEN, flash_data);
                    console.log(`FLASH: ${flash_data}`);
                    d.config.value = config_from_bytes(flash_data);
                    if (on_config_changed_callback != null) {
                        on_config_changed_callback(d);
                    }
                    let s = bytes_to_hex_string(flash_data, FLASH_MAX_LEN);
                    console.log(`Memory for device ${d.serial_str}: ${s}`);
                }
            } break;

            // write flash memory ACK
            case 0x4: {
                let d = midi_devices.value[device_index];
                if (d !== undefined) {
                    console.log(`Device ${d.serial_str} wrote flash data`);
                }
            } break;
        }
    }
}

let on_config_changed_callback = null;

function on_config_changed(callback) {
    on_config_changed_callback = callback;
}

function on_midi(midi_obj) {
    midi = midi_obj;
    midi.addEventListener('statechange', function (event) {
        console.log(`statechange: ${event.port.name} (${event.port.id}): ${event.port.state}`);
        init_devices();
    });
    init_devices();
}

export default {
    midi,
    midi_devices,
    init_devices,
    on_midi_message,
    on_config_changed,
    on_midi,
    toggle_device_led,
    read_flash,
    write_flash
}
