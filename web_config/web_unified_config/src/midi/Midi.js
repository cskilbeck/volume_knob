//////////////////////////////////////////////////////////////////////
// web midi stuff

import { ref, computed } from 'vue'
import { make_dummy_midi_ports } from './dummy_midi.js'

const CONFIG_LEN = 26;


// The 1st config byte = (device type << 4) | format version  (see firmware device.h)
//   device type: 0x0 = MIDI, 0x8 = HID   |   format version: 0..15
// MIDI keeps device type 0x0, so every value it ever shipped (0x01..0x0A) already
// conforms — no renumbering, backward compatible by construction.
const DEVTYPE_MIDI = 0x0

const config_device_type = (v) => (v >> 4) & 0xf
const config_format_version = (v) => v & 0xf

// current MIDI config: format version 10 (byte 0x0A)
const CONFIG_VERSION = 0x0A

// oldest MIDI format whose byte layout this UI can still parse — fields are
// append-only from here, landing in the old 'pad' region, so a newer UI keeps
// reading older devices and just hides controls they don't support.
const MIDI_MIN_READABLE_FORMAT = 0x9
// format that introduced rot_min/rot_max
const MIDI_FORMAT_ABS_RANGE = 0x0A

// can this UI read/write a device reporting this config byte?
function is_supported_config_version(v, type) {
    const t = type || KNOB_TYPE;
    return config_device_type(v) === t.device_type &&
        config_format_version(v) >= t.min_format &&
        config_format_version(v) <= t.max_format;
}

// does the firmware behind this config byte honour rot_min/rot_max?
function supports_abs_range(v) {
    return config_device_type(v) === DEVTYPE_MIDI &&
        config_format_version(v) >= MIDI_FORMAT_ABS_RANGE;
}

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

// Adapter only: returns four raw ADC readings — TIP sensed with RING driven
// high then low, then RING sensed with TIP driven high then low. Deliberately
// raw rather than a verdict, so the browser can show them and let the user
// interpret what the firmware cannot.
const sysex_request_probe = 0x06;

//const sysex_response_unused_01 = 0x01;
const sysex_response_device_id = 0x02;
const sysex_response_get_flash = 0x03;
const sysex_response_set_flash_ack = 0x04;
const sysex_response_probe = 0x06;

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

    // current button toggle state
    cf_toggle: 0x0400,

    // button's second value ('released') tracks rotation value (for e.g. mute/unmute)
    cf_button_tracks_rotation: 0x0800,

    // other rotary encoder type
    cf_rotate_reverse: 0x1000
};

//////////////////////////////////////////////////////////////////////

let default_flags = flags.cf_led_flash_on_limit | flags.cf_led_flash_on_click | flags.cf_acceleration_lsb | flags.cf_rotate_extended | flags.cf_btn_momentary;

const default_config = {
    config_version: CONFIG_VERSION, // config struct version - must be 1st byte!
    rot_control_msb: 7,             // Control Change index MSB,LSB for knob
    rot_control_lsb: 39,            // Control Change index MSB,LSB for knob
    btn_control_msb: 120,           // Control Change index MSB,LSB for button
    btn_control_lsb: 0,             // Control Change index MSB,LSB for button
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
    acceleration: 0,
    flags: default_flags,           // flags, see enum above
    rot_min: 0,                     // absolute-mode lower limit (0 => full range)
    rot_max: 0                      // absolute-mode upper limit (0 => full range)
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
    ["uint8", "acceleration"],
    ["uint16", "flags"],
    ["uint16", "rot_min"],
    ["uint16", "rot_max"]
];

// similar source code synchronization problem with the flags

// NOTE: this block must stay BELOW the knob's CONFIG_LEN, flags,
// default_config and config_map. It captures them by value at module
// evaluation time, so placing it above them throws a temporal dead zone
// error and the whole app renders a blank page.
//////////////////////////////////////////////////////////////////////
// TRS MIDI adapter (E:\dev\midi_adapter) — model 0x3323, device type 0x1.
//
// Same manufacturer and family as the knob, same SysEx transport and 7-bit
// packing, different config. See ADAPTER-INTEGRATION.md.

const DEVTYPE_ADAPTER = 0x1;

const ADAPTER_MODEL_NUMBER_LOW = 0x23;   // 0x3323
const ADAPTER_MODEL_NUMBER_HIGH = 0x33;

const ADAPTER_CONFIG_LEN = 32;
const ADAPTER_CONFIG_VERSION = (DEVTYPE_ADAPTER << 4) | 0x0;

// Mirrors adapter_config_t in midi_adapter/firmware/src/config.h, which is
// __attribute__((packed)) precisely so this flat map is valid. config.h carries
// a _Static_assert on the struct size — but that cannot catch a REORDERING, so
// if the struct changes, change this with it.
const adapter_config_map = [
    ["uint8", "version"],
    ["uint8", "mode"],
    ["uint8", "channel"],
    ["uint8", "cc_msb"],
    ["uint8", "cc_lsb"],
    ["uint16", "flags"],
    ["uint16", "in_min"],
    ["uint16", "in_max"],
    ["uint16", "out_min"],
    ["uint16", "out_max"],
    ["uint8", "deadband"],
    ["uint8", "smoothing"],
    ["uint16", "sw_threshold"],
    ["uint16", "sw_hysteresis"],
    ["uint16", "sw_value_on"],
    ["uint16", "sw_value_off"],
    ["uint8", "led_brightness"],
    ["uint8", "pad0"], ["uint8", "pad1"], ["uint8", "pad2"],
    ["uint8", "pad3"], ["uint8", "pad4"], ["uint8", "pad5"],
];

const adapter_flags = {
    af_wiper_on_ring: 0x0001,
    af_invert: 0x0002,
    af_14bit: 0x0004,
    af_sw_on_ring: 0x0008,
    af_sw_norm_closed: 0x0010,
    af_sw_toggle: 0x0020,
    af_led_activity: 0x0040,
};

export const ADAPTER_MODE = { midi: 0, expression: 1, switch: 2 };

const adapter_default_config = {
    version: ADAPTER_CONFIG_VERSION,
    mode: ADAPTER_MODE.midi,
    channel: 0,
    cc_msb: 11,
    cc_lsb: 43,
    flags: adapter_flags.af_led_activity,
    in_min: 0, in_max: 4095,
    out_min: 0, out_max: 16383,
    deadband: 8, smoothing: 2,
    sw_threshold: 2048, sw_hysteresis: 256,
    sw_value_on: 16383, sw_value_off: 0,
    led_brightness: 100,
    pad0: 0, pad1: 0, pad2: 0, pad3: 0, pad4: 0, pad5: 0,
};

//////////////////////////////////////////////////////////////////////
// Device registry, keyed by model number.
//
// Everything that differs between devices in this family lives here, so that
// on_device_id_response() can LOOK UP a model rather than compare against one
// hardcoded value — which is what used to make anything but the knob invisible.

const DEVICE_TYPES = [
    {
        label: "Tiny MIDI Knob",
        component: "MidiDevice",
        device_type: DEVTYPE_MIDI,
        model: [MIDI_MODEL_NUMBER_LOW, MIDI_MODEL_NUMBER_HIGH],
        config_len: CONFIG_LEN,
        config_map: config_map,
        default_config: default_config,
        flags: flags,
        min_format: MIDI_MIN_READABLE_FORMAT,
        max_format: config_format_version(CONFIG_VERSION),
        // The knob can be put into its bootloader over SysEx; the adapter's
        // CH32V203 cannot, so this gates the UI rather than the UI assuming.
        can_flash: true,
    },
    {
        label: "TRS MIDI Adapter",
        component: "MidiAdapterDevice",
        device_type: DEVTYPE_ADAPTER,
        model: [ADAPTER_MODEL_NUMBER_LOW, ADAPTER_MODEL_NUMBER_HIGH],
        config_len: ADAPTER_CONFIG_LEN,
        config_map: adapter_config_map,
        default_config: adapter_default_config,
        flags: adapter_flags,
        min_format: 0x0,
        max_format: 0x0,
        can_flash: false,
    },
];

const KNOB_TYPE = DEVICE_TYPES[0];
const ADAPTER_TYPE = DEVICE_TYPES[1];

function device_type_for_model(lo, hi) {
    return DEVICE_TYPES.find(t => t.model[0] === lo && t.model[1] === hi) || null;
}

//////////////////////////////////////////////////////////////////////
// this is super nasty - marshal/unmarshal from bytes to config struct

function config_from_bytes(bytes, type) {

    const t = type || KNOB_TYPE;
    let def_config = {};
    let new_config = {};
    Object.assign(def_config, t.default_config);
    Object.assign(new_config, t.default_config);

    if (!is_supported_config_version(bytes[0], t)) {
        return def_config;
    }

    if (bytes.BYTES_PER_ELEMENT != 1) {
        return def_config;
    }

    let field_offset = 0;
    for (const field of t.config_map) {
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
            return def_config;
        }
        let value = 0;
        for (let i = 0; i < field_size; ++i) {
            value |= bytes[field_offset] << (i * 8);
            field_offset += 1;
        }
        new_config[field_name] = value;
    }
    return new_config;
}

//////////////////////////////////////////////////////////////////////

function bytes_from_config(config, type) {

    const t = type || KNOB_TYPE;
    let bytes = new Uint8Array(t.config_len);

    let field_offset = 0;
    for (const field of t.config_map) {
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
        console.log(`Flash led for device ${device.name}`);
        send_midi(device, [0xF0, 0x7E, device.device_index & 0x7f, 0x06, sysex_request_toggle_led, 0xF7]);
    }
}

//////////////////////////////////////////////////////////////////////

function flash_mode(index) {

    const device = midi_devices.value[index];
    if (device === undefined) {
        console.log(`Can't find device ${index}`);
    } else {
        console.log(`Enter flash mode for device ${device.name}`);
        send_midi(device, [0xF0, 0x7E, device.device_index & 0x7f, 0x06, sysex_request_bootloader, 0xF7]);
    }
}

//////////////////////////////////////////////////////////////////////
// The identity reply's optional tail: ASCII hex, from data[14] up to the
// closing 0xF7. Anything that is not plain hex is treated as absent rather
// than shown to the user, so a future device that appends something else here
// cannot put junk on the screen.

function serial_from_identity(data) {

    const end = data.length - 1;        // the trailing 0xF7
    if (end <= 14) {
        return null;
    }
    const s = String.fromCharCode(...data.slice(14, end));
    return /^[0-9A-F]+$/.test(s) ? s : null;
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

    // check MANUFACTURER and FAMILY, then look the MODEL up in the registry.
    // Comparing against one hardcoded model is what used to make every device
    // in this family except the knob invisible.
    if (!(data[5] == MIDI_MANUFACTURER_ID &&
        data[6] == MIDI_FAMILY_CODE_LOW &&
        data[7] == MIDI_FAMILY_CODE_HIGH)) {

        console.log(`Unrecognized manufacturer/family, ignoring...`);
        return;
    }

    const type = device_type_for_model(data[8], data[9]);
    if (type === null) {
        console.log(`Unrecognized model ${data[9].toString(16)}${data[8].toString(16)}, ignoring...`);
        return;
    }

    // Serial number, when the device sends one. Firmware new enough to have it
    // appends the same ASCII hex string that the USB serial descriptor carries,
    // after the version bytes — ASCII hex is 7-bit clean by construction, so it
    // needs no packing, and appending it leaves older clients (which read fixed
    // offsets up to data[13]) working unchanged. Older firmware simply stops at
    // data[13] and has no serial, which is not an error.
    //
    // Set BEFORE device.type, which is what makes the panel render: the panel
    // reads the serial during setup, so it has to be there first.
    device.serial = serial_from_identity(data);

    // Defaults only when the type is first learned. Identity replies can
    // arrive more than once (init_devices asks, and MidiDevice.vue asks again
    // when it mounts), and re-defaulting here would discard a config that has
    // already been read back from the device.
    if (device.type !== type) {
        device.type = type;
        Object.assign(device.config, type.default_config);
    }

    // get the firmware version from the device id response

    let b0 = data[10] || 0;
    let b1 = data[11] || 0;
    let b2 = data[12] || 0;
    let b3 = data[13] || 0;

    device.firmware_version = b3 | (b2 << 7) | (b1 << 14) | (b0 << 21);
    device.firmware_version_str = `${b3}.${b2}.${b1}.${b0}`;

    // add a new device to the array of midi_devices

    console.log(`Found device ${device.name}, FW version # ${device.firmware_version_str}, ${midi_devices.value.length} device(s) so far...`);

    device.input = input_port;
    device.input_id = input_port.id;

    // get the config — small delay so the firmware finishes handling the
    // device-ID reply before we hit it with the next SysEx request.
    setTimeout(() => read_flash(reply_index), 50);
}

//////////////////////////////////////////////////////////////////////
// send a request for the flash contents

function probe_device(index) {

    const device = midi_devices.value[index];
    if (device === undefined) {
        console.log(`probe_device: No such device ${index}`);
        return;
    }
    send_midi(device, [0xF0, 0x7E, device.device_index & 0x7f, 0x06, sysex_request_probe, 0xF7]);
}

//////////////////////////////////////////////////////////////////////

function read_flash(index) {

    const device = midi_devices.value[index];
    if (device === undefined) {
        console.log(`read_flash: No such device ${index}`);
        return;
    }
    send_midi(device, [0xF0, 0x7E, device.device_index & 0x7f, 0x06, sysex_request_get_flash, 0xF7]);
}

//////////////////////////////////////////////////////////////////////
// send new flash contents

function write_flash(index) {

    const device = midi_devices.value[index];
    if (device === undefined) {
        console.log(`write_flash: No such device ${index}`);
        return;
    }
    const type = device.type || KNOB_TYPE;
    let data_7bits = bytes_to_bits7(bytes_from_config(device.config, type), 0, type.config_len);
    if (data_7bits !== null) {
        send_midi(device, [0xF0, 0x7E, device.device_index & 0x7f, 0x06, sysex_request_set_flash].concat(Array.from(data_7bits)).concat([0xF7]));
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
//
// Every request carries the target's device_index in byte 2, and replies echo
// it — that is how on_midi_message tells one device's answers from another's.
// These all used to send a hardcoded 0x00, which worked only while the device
// being addressed happened to be first in the list. With a knob at 0 and an
// adapter at 1, the adapter answered as 0: its config was unpacked into the
// knob's panel, and its calibration readings went to a device with no
// calibration UI, which ignored them silently.

function send_midi(midi_device, data) {

    // console.log(`SEND: ${bytes_to_hex_string(data, data.length, " ")}`);
    midi_device.output.send(data);
}

//////////////////////////////////////////////////////////////////////

function init_devices() {

    console.log(`init_devices`);

    // Snapshot existing devices by KEY so we can REUSE them across rescans.
    // Replacing them with fresh objects would orphan the callbacks (on_control_change
    // etc.) that MidiDevice.vue set on the original objects — Vue's keyed v-for
    // reuses the component instance for the same device.key, so setup doesn't
    // re-run and the new objects never get callbacks installed. This also
    // preserves the demo device through rescans.
    //
    // The key is the output port's id, NOT its name. Two adapters both report
    // themselves as "midi_adapter", so a name-keyed map keeps only the last of
    // them: one device gets reused and the other silently rebuilt without its
    // callbacks. Port ids are unique per port, which is exactly what is wanted.
    const existing_by_key = {};
    for (const d of midi_devices.value) {
        existing_by_key[d.key] = d;
    }

    midi_devices.value = [];
    device_index = 0;

    scanned.done = true;

    console.log(`${midi.inputs.size} inputs, ${midi.outputs.size} outputs`);

    for (const input of midi.inputs.values()) {
        console.log(`Found ${input.name} at ${input.id}`);
        input.removeEventListener("midimessage", on_midi_message);
        input.addEventListener("midimessage", on_midi_message);
    }

    for (const output of midi.outputs.values()) {

        console.log(`Found ${output.name} at ${output.id}`);

        let device = existing_by_key[output.id];

        if (device) {
            // Reuse — refresh the output reference (a new MIDIOutput instance
            // may have replaced the old one) and renumber.
            device.output = output;
            device.device_index = device_index;
            delete existing_by_key[output.id];
        } else {
            device = {
                kind: 'midi',
                key: output.id,     // identity; name is a display label only
                device_index: device_index,
                firmware_version: 0x00000000,
                firmware_version_str: "0.0.0.0",
                input: null,            // inputs get assigned when replies come back
                output: output,
                name: output.name,
                config: {},
                on_config_loaded: null,
                on_config_saved: null,
                on_control_change: null,
                on_midi_in: null,
                on_probe: null,
                type: null,         // set from the identity reply
            };

            Object.assign(device.config, default_config);

            Object.defineProperty(device, 'active', {
                get() {
                    return this.input != null && this.input.state == 'connected';
                }
            });
        }

        midi_devices.value[device_index] = device;

        // Ask who it is, here, rather than leaving it to the component that
        // renders it. DeviceList picks a component from device.type, and
        // device.type comes from the identity REPLY — so a component-driven
        // request can never happen for a device we have not identified yet.
        connect_device(device);

        device_index += 1;
    }

    // Anything left in existing_by_key is either the demo or a real device
    // whose output is no longer enumerated (unplugged). Keep the demo, drop
    // the rest.
    for (const key in existing_by_key) {
        const d = existing_by_key[key];
        if (d.demo) {
            d.device_index = device_index;
            if (d._set_port_index) d._set_port_index(device_index);
            midi_devices.value[device_index] = d;
            device_index += 1;
        }
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

function device_from_input_port(port) {
    // Compared by object identity, not by name. This routes every non-SysEx
    // message — the monitor and every CC — so matching on name sends both
    // adapters' traffic to whichever one is first in the list, leaving the
    // other panel dead. The event fires on the port object we stored, so
    // identity is exact and free.
    for (let d of midi_devices.value) {
        if (d.input === port) {
            return d;
        }
    }
    return null;
}

//////////////////////////////////////////////////////////////////////

function on_midi_message(event) {

    let input_port = event.target;

    console.log(event);

    const data = event.data;

    console.log(`RECV: ${bytes_to_hex_string(data, data.length, " ")}`);

    let midi_status = data[0] & 0xf0;

    // Anything that is not SysEx is traffic the device is producing, which for
    // the TRS adapter is the entire point of it. Routed wholesale so a
    // component can monitor it.
    //
    // Tested on data[0] rather than midi_status because system real-time bytes
    // (0xF8 clock and friends) also have 0xF0 in their high nibble and would
    // otherwise be mistaken for the start of a SysEx message.
    if (data[0] !== 0xF0) {
        const listener = device_from_input_port(input_port);
        if (listener != null && listener.on_midi_in != null) {
            listener.on_midi_in(data);
        }
    }

    switch (midi_status) {

        // B0 is control change
        case 0xB0: {

            let device = device_from_input_port(input_port);
            if (device != null && device.on_control_change != null) {
                device.on_control_change(data[0] & 0xf, data[1], data[2]);
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
                            const dt = d.type || KNOB_TYPE;
                            let flash_data = bits7_to_bytes(data, 5, dt.config_len);
                            d.config = config_from_bytes(flash_data, dt);
                            let s = bytes_to_hex_string(flash_data, dt.config_len);
                            console.log(`Memory for device ${d.name}: ${s}`);

                            if (d.on_config_loaded != null) {
                                d.on_config_loaded();
                            }
                        }
                    } break;

                    // raw probe readings (adapter)
                    case sysex_response_probe: {
                        let d = midi_devices.value[device_index];
                        if (d !== undefined && d.on_probe != null) {
                            // 8 bytes = four uint16, little endian
                            d.on_probe(bits7_to_bytes(data, 5, 8));
                        }
                    } break;

                    // write flash memory ACK
                    case sysex_response_set_flash_ack: {
                        let d = midi_devices.value[device_index];
                        if (d !== undefined) {
                            console.log(`Device ${d.name} wrote flash data`);
                            if (d.on_config_saved != null) {
                                d.on_config_saved(d);
                            }
                        }
                    } break;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////

function on_state_change(event) {

    console.log(`statechange: ${event.port.name} (${event.port.id}): ${event.port.state}`);

    switch (event.port.state) {

        case 'disconnected':
            for (let device of midi_devices.value) {
                if (device.input && device.input.id == event.port.id) {
                    device.input = null;
                } else if (device.output && device.output.id == event.port.id) {
                    device.output = null;
                }
            }
            break;

        case 'connected': {

            // Matched by port id, the same way the 'disconnected' branch above
            // already does it. Matching on name assigns the returning port to
            // EVERY device sharing that name — there is no break — so two
            // adapters, both called "midi_adapter", end up pointing at one
            // physical device.
            //
            // An input's id differs from its device's output id, so the input
            // id is remembered separately when the identity reply first pairs
            // them, and deliberately not cleared on disconnect: it is the only
            // thing left to recognise the port by when it comes back.
            const wanted = event.port.type === 'input'
                ? (d) => d.input_id === event.port.id
                : (d) => d.key === event.port.id;

            let device = midi_devices.value.find(wanted);

            // Fallback for a host that does not hand the same id back across a
            // replug. Restricted to devices with no port of this type bound,
            // which keeps it unambiguous even among identical names: a device
            // that already has its input cannot claim another one.
            if (device === undefined) {
                device = midi_devices.value.find(d => d.name === event.port.name &&
                    (event.port.type === 'input' ? d.input == null : d.output == null));
            }

            if (device !== undefined) {
                switch (event.port.type) {
                    case 'input':
                        device.input = event.port;
                        device.input_id = event.port.id;
                        break;
                    case 'output':
                        // device.key is deliberately NOT updated to a new id
                        // here: it is the Vue key, and changing it mid-flight
                        // tears down and rebuilds the panel. If the id really
                        // did change, the next scan picks it up cleanly.
                        device.output = event.port;
                        break;
                }
                if (device.input && device.output) {
                    console.log(`Reconnect: ${device.name}`);
                    device.input.removeEventListener("midimessage", on_midi_message);
                    device.input.addEventListener("midimessage", on_midi_message);
                }
            }
        } break;
    }
}

//////////////////////////////////////////////////////////////////////

function on_midi_startup(midi_obj) {

    midi = midi_obj;

    midi.removeEventListener('statechange', on_state_change);
    midi.addEventListener('statechange', on_state_change);
}

//////////////////////////////////////////////////////////////////////

let auto_rotate_interval = null;

function start_auto_rotate(device, ports) {
    let phase = 0;
    auto_rotate_interval = setInterval(() => {
        // Don't fight the user mid-edit. Document.activeElement is the focused
        // form control; if any is focused, hold off this tick.
        const el = document.activeElement;
        if (el && (el.tagName === 'INPUT' || el.tagName === 'SELECT' || el.tagName === 'TEXTAREA')) {
            return;
        }
        phase += 0.06;
        const cfg = device.config || default_config;
        const channel = (cfg.channels || 0) & 0xf;
        const cc = cfg.rot_control_msb ?? 7;
        const val = Math.floor((Math.sin(phase) + 1) * 63);  // 0..126
        ports.emit_cc(channel, cc, val);
    }, 120);
}

function stop_auto_rotate() {
    if (auto_rotate_interval) {
        clearInterval(auto_rotate_interval);
        auto_rotate_interval = null;
    }
}

// type defaults to the knob so the existing "+ Demo MIDI" button is unchanged.
function add_dummy_device(type) {
    const t = type || KNOB_TYPE;
    const demo_name = `Demo ${t.label}`;

    if (midi_devices.value.some(d => d.demo && d.name === demo_name)) return;

    const ports = make_dummy_midi_ports({
        name: demo_name,
        default_config: t.default_config,
        // Bound to this device type: the dummy marshals its own stored config
        // and must use the right map, not the knob's.
        bytes_from_config: (c) => bytes_from_config(c, t),
        bytes_to_bits7,
        bits7_to_bytes,
        CONFIG_LEN: t.config_len,
        MIDI_MANUFACTURER_ID,
        MIDI_FAMILY_CODE_LOW,
        MIDI_FAMILY_CODE_HIGH,
        MIDI_MODEL_NUMBER_LOW: t.model[0],
        MIDI_MODEL_NUMBER_HIGH: t.model[1],
        sysex_request_device_id,
        sysex_request_toggle_led,
        sysex_request_get_flash,
        sysex_request_set_flash,
        sysex_request_bootloader,
        sysex_response_device_id,
        sysex_response_get_flash,
        sysex_response_set_flash_ack,
    });

    const idx = device_index;
    ports.set_device_index(idx);

    const device = {
        kind: 'midi',
        demo: true,
        key: ports.output.id,
        device_index: idx,
        firmware_version: 0x00000000,
        firmware_version_str: "0.0.0.0",
        input: ports.input,
        output: ports.output,
        name: demo_name,
        type: t,
        config: {},
        on_config_loaded: null,
        on_config_saved: null,
        on_control_change: null,
        on_midi_in: null,
        _set_port_index: ports.set_device_index,
    };

    Object.assign(device.config, t.default_config);

    Object.defineProperty(device, 'active', {
        get() {
            return this.input != null && this.input.state == 'connected';
        }
    });

    // A MIDI monitor with nothing in it is a poor preview, so the adapter demo
    // plays a little. Stopped in remove_dummy_device().
    if (t === ADAPTER_TYPE) {
        let step = 0;
        device._demo_timer = setInterval(() => {
            const notes = [60, 64, 67, 72];
            const n = notes[step % notes.length];
            ports.emit_raw([0x90, n, 100]);
            setTimeout(() => ports.emit_raw([0x80, n, 0]), 300);
            ports.emit_raw([0xB0, 11, (step * 8) & 0x7f]);
            step++;
        }, 900);
    }

    ports.input.removeEventListener("midimessage", on_midi_message);
    ports.input.addEventListener("midimessage", on_midi_message);

    midi_devices.value[idx] = device;
    device_index += 1;

    scanned.done = true;

    // Drive the existing handshake: device-ID → flash-read → on_config_loaded.
    connect_device(device);

    start_auto_rotate(device, ports);
}

function remove_dummy_device(type) {
    const t = type || KNOB_TYPE;

    // Only the knob demo has a rotation animation running.
    if (t === KNOB_TYPE) {
        stop_auto_rotate();
    }
    const demo_name = `Demo ${t.label}`;
    const i = midi_devices.value.findIndex(d => d.demo && d.name === demo_name);

    if (i >= 0) {
        if (midi_devices.value[i]._demo_timer) {
            clearInterval(midi_devices.value[i]._demo_timer);
        }
        midi_devices.value.splice(i, 1);
        // If the dummy was last, drop device_index back so the next demo gets the same slot.
        if (i === device_index - 1) device_index -= 1;
    }
}

const has_dummy = computed(() => midi_devices.value.some(d => d.name === `Demo ${KNOB_TYPE.label}`));
const has_dummy_adapter = computed(
    () => midi_devices.value.some(d => d.name === `Demo ${ADAPTER_TYPE.label}`));

function add_dummy_adapter() { add_dummy_device(ADAPTER_TYPE); }
function remove_dummy_adapter() { remove_dummy_device(ADAPTER_TYPE); }

//////////////////////////////////////////////////////////////////////

export default {
    midi,
    midi_devices,
    scanned,
    on_midi_startup,
    init_devices,
    connect_device,
    toggle_device_connection,
    flash_device_led,
    flash_mode,
    read_flash,
    write_flash,
    probe_device,
    default_config,
    flags,
    supports_abs_range,
    add_dummy_device,
    remove_dummy_device,
    has_dummy,
    add_dummy_adapter,
    remove_dummy_adapter,
    has_dummy_adapter,
    DEVICE_TYPES,
}
