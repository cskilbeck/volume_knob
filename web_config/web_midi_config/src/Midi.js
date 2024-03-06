//////////////////////////////////////////////////////////////////////
// web midi stuff

import { ref, toRaw } from 'vue'

const FLASH_MAX_LEN = 26;

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
// output ports which have been sent a device id request
// position in the array is device_index

let output_ports = [];

//////////////////////////////////////////////////////////////////////
// next midi device index

let device_index = 0;

//////////////////////////////////////////////////////////////////////

let default_config = {
    version: 0,
    rot_control_high: 0,
    rot_control_low: 0,
    btn_control_high: 0,
    btn_control_low: 0,
    rot_channel: 0,
    btn_channel: 0,
    rot_zero_point: 0,
    rot_delta: 0,
    rot_limit_low: 0,
    rot_limit_high: 0,
    rot_current_value: 0,
    btn_value_1: 0,
    btn_value_2: 0,
    cf_rotate_extended: false,
    cf_rotate_relative: false,
    cf_led_invert: false,
    cf_led_flash_on_rot: false,
    cf_led_flash_on_limit: false,
    cf_btn_momentary: false,
    cf_btn_extended: false,
    cf_led_flash_on_click: false,
    cf_led_flash_on_release: false,
    cf_led_track_button_toggle: false,
    cf_acceleration: false
};

const sysex_request_device_id = 0x01;
const sysex_request_toggle_led = 0x02;
const sysex_request_get_flash = 0x03;
const sysex_request_set_flash = 0x04;
const sysex_request_bootloader = 0x05;
const sysex_request_firmware_version = 0x06;

const sysex_response_unused_01 = 0x01;
const sysex_response_device_id = 0x02;
const sysex_response_get_flash = 0x03;
const sysex_response_set_flash_ack = 0x04;
const sysex_response_firmware_version = 0x05;

/*

//////////////////////////////////////////////////////////////////////
// flags in config.cf_flags

typedef enum config_flags
{
    // rotate modifies both CC MSB and LSB
    cf_rotate_extended = 0x0001,

    // rotate sends relative changes
    cf_rotate_relative = 0x0002,

    // led on means off and vice versa
    cf_led_invert = 0x0004,

    // flash when knob is rotated
    cf_led_flash_on_rot = 0x0008,

    // flash when rotation limit is hit (if not cf_rotate_relative)
    cf_led_flash_on_limit = 0x0010,

    // btn sets value1/value2 based on state of button rather than toggling between them
    cf_btn_momentary = 0x0020,

    // button modifies both CC MSB and LSB
    cf_btn_extended = 0x0040,

    // flash led when button is clicked
    cf_led_flash_on_click = 0x0080,

    // flash led when button is released
    cf_led_flash_on_release = 0x0100,

    // led tracks state of button
    cf_led_track_button_toggle = 0x0200,

    // two bits for acceleration (so 4 options: off, low, med, high)
    cf_acceleration_lsb = 0x0400,
    cf_acceleration_msb = 0x0800,
};

//////////////////////////////////////////////////////////////////////
// sizeof config_t must be FLASH_LEN

typedef struct config
{
    // 0  uint8 version;
    // 1  uint8 rot_control[2];
    // 3  uint8 btn_control[2];
    // 5  uint16 btn_value[2];
    // 9  uint8 channels;
    // 10 uint16 rot_zero_point;
    // 12 uint16 rot_delta;
    // 14 uint16 rot_limit_low;
    // 16 uint16 rot_limit_high;
    // 18 uint16 rot_current_value;
    // 20 uint16 flags;
    // 22 uint8 pad[FLASH_LEN - 20];

} config_t;

*/

//////////////////////////////////////////////////////////////////////
// this is a super nasty
// the temptation is to build a system for keeping the structure
// declaration in the firmware source code in sync with this stuff
// rather than all this hard coded offset nonsense, which would
// involve some fiddly kind of macro / include thingy. Maybe...

function config_from_bytes(bytes) {

    let config = default_config;

    config.version = bytes[0] & 0xff;
    config.rot_control_high = bytes[1] & 0x7f;
    config.rot_control_low = bytes[2] & 0x7f;
    config.btn_control_high = bytes[3] & 0x7f;
    config.btn_control_low = bytes[4] & 0x7f;
    config.btn_value_1 = bytes[5] & 0xff;
    config.btn_value_1 |= (bytes[6] << 8) & 0x3f;
    config.btn_value_2 = bytes[7] & 0xff;
    config.btn_value_2 |= (bytes[8] << 8) & 0x3f;
    config.rot_channel = bytes[9] & 0xf;
    config.btn_channel = (bytes[9] >> 4) & 0xf;
    config.rot_zero_point = bytes[10] & 0xff;
    config.rot_zero_point |= (bytes[11] & 0x3f) << 8;
    config.rot_delta = bytes[12] & 0xff;
    config.rot_delta |= (bytes[13] & 0x3f) << 8;
    config.rot_limit_low = bytes[14] & 0xff;
    config.rot_limit_low |= (bytes[15] & 0x3f) << 8;
    config.rot_limit_high = bytes[16] & 0xff;
    config.rot_limit_high |= (bytes[17] & 0x3f) << 8;
    config.rot_current_value = bytes[18] & 0xff;
    config.rot_current_value |= (bytes[19] & 0x3f) << 8;

    let flags = bytes[20] & 0xff;
    flags |= (bytes[21] & 0xff) << 8;

    config.cf_rotate_extended = (flags & 0x0001) != 0;
    config.cf_rotate_relative = (flags & 0x0002) != 0;
    config.cf_led_invert = (flags & 0x0004) != 0;
    config.cf_led_flash_on_rot = (flags & 0x0008) != 0;
    config.cf_led_flash_on_limit = (flags & 0x0010) != 0;
    config.cf_btn_momentary = (flags & 0x0020) != 0;
    config.cf_btn_extended = (flags & 0x0040) != 0;
    config.cf_led_flash_on_click = (flags & 0x0080) != 0;
    config.cf_led_flash_on_release = (flags & 0x0100) != 0;
    config.cf_led_track_button_toggle = (flags & 0x0200) != 0;
    config.cf_acceleration = (flags & 0x0C00) >> 10;

    console.log(config);

    return config;
}

//////////////////////////////////////////////////////////////////////

function bytes_from_config(config) {
    let bytes = new Array(26).fill(0);

    bytes[0] = config.version & 0xff;
    bytes[1] = config.rot_control_high & 0x7f;
    bytes[2] = config.rot_control_low & 0x7f;
    bytes[3] = config.btn_control_high & 0x7f;
    bytes[4] = config.btn_control_low & 0x7f;
    bytes[5] = config.btn_value_1 & 0xff;
    bytes[6] = (config.btn_value_1 >> 8) & 0x3f;
    bytes[7] = config.btn_value_2 & 0xff;
    bytes[8] = (config.btn_value_2 >> 8) & 0x3f;
    bytes[9] = config.rot_channel & 0xf;
    bytes[9] |= (config.btn_channel & 0xf) << 4;
    bytes[10] = config.rot_zero_point & 0xff;
    bytes[11] = (config.rot_zero_point >> 8) & 0x3f;
    bytes[12] = config.rot_delta & 0xff;
    bytes[13] = (config.rot_delta >> 8) & 0x3f;
    bytes[14] = config.rot_limit_low & 0xff;
    bytes[15] = (config.rot_limit_low >> 8) & 0x3f;
    bytes[16] = config.rot_limit_high & 0xff;
    bytes[17] = (config.rot_limit_high >> 8) & 0x3f;
    bytes[18] = config.rot_current_value & 0xff;
    bytes[19] = (config.rot_current_value >> 8) & 0x3f;

    let flags = 0;

    flags |= config.cf_rotate_extended ? 0x0001 : 0;
    flags |= config.cf_rotate_relative ? 0x0002 : 0;
    flags |= config.cf_led_invert ? 0x0004 : 0;
    flags |= config.cf_led_flash_on_rot ? 0x0008 : 0;
    flags |= config.cf_led_flash_on_limit ? 0x0010 : 0;
    flags |= config.cf_btn_momentary ? 0x0020 : 0;
    flags |= config.cf_btn_extended ? 0x0040 : 0;
    flags |= config.cf_led_flash_on_click ? 0x0080 : 0;
    flags |= config.cf_led_flash_on_release ? 0x0100 : 0;
    flags |= config.cf_led_track_button_toggle ? 0x0200 : 0;
    flags |= (config.cf_acceleration & 3) << 10;

    bytes[20] = flags & 0xff;
    bytes[21] = (flags >> 8) & 0xff;

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

const MIDI_MANUFACTURER_ID = 0x36;    // Cheetah Marketing, defunct?

const MIDI_FAMILY_CODE_LOW = 0x44;   // 0x5544
const MIDI_FAMILY_CODE_HIGH = 0x55;

const MIDI_MODEL_NUMBER_LOW = 0x22;   // 0x3322
const MIDI_MODEL_NUMBER_HIGH = 0x33;

//////////////////////////////////////////////////////////////////////

function on_device_id_response(input_port, data) {

    let reply_index = data[2];
    let device = midi_devices.value[reply_index];
    if (device == undefined) {
        console.log(`Midi from unknown device: ${device.name}`);
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
    let data_7bits = [];
    bytes_to_bits7(bytes_from_config(device.config.value), 0, FLASH_MAX_LEN, data_7bits);
    send_midi(device, [0xF0, 0x7E, 0x00, 0x06, sysex_request_set_flash].concat(data_7bits).concat([0xF7]));
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

    scanned.done = true;

    console.log(`${midi.inputs.size} inputs, ${midi.outputs.size} outputs`);

    // initial setup of callbacks for midi messages

    for (const input of midi.inputs.values()) {
        console.log(`Found ${input.name} at ${input.id}`);
        input.addEventListener("midimessage", on_midi_message);
    }

    // midi_devices[] is based on output ports
    // inputs get assigned when replies come back

    for (const output of midi.outputs.values()) {

        console.log(`Found ${output.name} at ${output.id}`);

        let device = {
            device_index: device_index,
            serial_number: 0,
            serial_str: "#######",
            input: null,
            output: output,
            name: output.name,
            firmware_version: 0x0000,
            config: ref({})
        };
        device.config.value = default_config;

        Object.defineProperty(device, 'active', {
            get() {
                return this.input != null && this.input.state == 'connected';
            }
        });

        midi_devices.value[device_index] = device;

        // for looking up the device when the response arrives
        //output_ports[device_index] = output;
        //output.send([0xF0, 0x7E, device_index & 0x7f, 0x06, sysex_request_device_id, 0xF7]);

        device_index += 1;
    }
    console.log(`init devices scanned ${device_index} devices`);
    console.log(midi_devices);
}

//////////////////////////////////////////////////////////////////////

function toggle_device_connection(device_index) {
    console.log(`Toggle device ${device_index} connection`);
    let d = midi_devices.value[device_index];
    console.log(d);
    if (d != null) {
        if (d.active) {
            console.log(`Closing ${d.input.name}:${d.input.state}`);
            d.input.close();
            d.output.close();
            console.log("Closed");
        } else {
            d.output.send([0xF0, 0x7E, device_index & 0x7f, 0x06, sysex_request_device_id, 0xF7]);
        }
    }
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

function on_midi_message(event) {

    let input_port = event.target;

    console.log(event);

    const data = event.data;

    console.log(`RECV: ${bytes_to_hex_string(data, data.length, " ")}`);

    let device_index = get_sysex_device_index(data);

    if (device_index != undefined) {

        switch (data[4]) {

            // device ID response
            case sysex_response_device_id: {
                on_device_id_response(input_port, data);
            } break;

            // read flash memory response
            case sysex_response_get_flash: {
                let d = midi_devices.value[device_index];
                if (d !== undefined) {
                    let flash_data = [];
                    bits7_to_bytes(data, 5, FLASH_MAX_LEN, flash_data);
                    console.log(`FLASH: ${bytes_to_hex_string(flash_data, flash_data.length, " ")}`);
                    let cfg = config_from_bytes(flash_data);
                    d.config.value = config_from_bytes(flash_data);
                    if (on_config_changed_callback != null) {
                        on_config_changed_callback(d);
                    }
                    let s = bytes_to_hex_string(flash_data, FLASH_MAX_LEN);
                    console.log(`Memory for device ${d.serial_str}: ${s}`);

                    // get fw version
                    send_midi(d, [0xF0, 0x7E, 0x00, 0x06, sysex_request_firmware_version, 0xF7]);
                }
            } break;

            // write flash memory ACK
            case sysex_response_set_flash_ack: {
                let d = midi_devices.value[device_index];
                if (d !== undefined) {
                    console.log(`Device ${d.serial_str} wrote flash data`);
                }
            } break;

            case sysex_response_firmware_version: {
                let d = midi_devices.value[device_index];
                if (d !== undefined) {
                    d.firmware_version = (data[5] & 0x7f) | ((data[6] & 0x7f) << 8);
                    console.log(`Device ${d.serial_str} has flash version: ${data[6]}.${data[5]}`);
                }
            } break;
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

let on_config_changed_callback = null;

function on_config_changed(callback) {
    on_config_changed_callback = callback;
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
    toggle_device_connection,
    on_config_changed,
    flash_device_led,
    flash_mode,
    read_flash,
    write_flash,
    get_config_json
}
