<script setup>

import { toRaw, watch, ref, getCurrentInstance } from 'vue';

import Toggle from './Toggle.vue'
import Modal from './Modal.vue'
import CCDropDown from './CCDropDown.vue';

import midi from '../Midi.js'

import fileDownload from 'js-file-download';

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

// 3 modal dialogs
const flashModal = ref(false);
const importModal = ref(false);
const disconnectModal = ref(false);

// is the current config different from what's on the device?
let config_changed = ref(false);

// this is the config on the device to compare against
let stored_config = null;

// noddy compare for config objects

function shallowEqual(object1, object2) {

    // if either are null, equality is false

    const keys1 = Object.keys(object1);
    const keys2 = Object.keys(object2);

    if (keys1.length !== keys2.length) {
        return false;
    }

    for (let key of keys1) {
        if (object1[key] != object2[key]) {
            return false;
        }
    }
    return true;
}

let that = getCurrentInstance();

// for petes sake, this absolutely sucks:
// manually create refs for everything in the config
// because I can't make the ref/reactive/vue thing work
// properly for nested objects etc

// this is at least bulletproof

// flags

let rotate_extended = ref(false);
let rotate_relative = ref(false);
let led_invert = ref(false);
let led_flash_on_rot = ref(false);
let led_flash_on_limit = ref(false);
let btn_momentary = ref(false);
let btn_extended = ref(false);
let led_flash_on_click = ref(false);
let led_flash_on_release = ref(false);
let led_track_button_toggle = ref(false);
let toggle = ref(false);
let button_tracks_rotation = ref(false);

// acceleration is a bit special, uses 2 bits in the flags

let acceleration = ref(0);

// fields

let config_version = ref(0);
let rot_control_msb = ref(0);
let rot_control_lsb = ref(0);
let btn_control_msb = ref(0);
let btn_control_lsb = ref(0);
let btn_value_a_14 = ref(0);
let btn_value_b_14 = ref(0);
let btn_value_a_7 = ref(0);
let btn_value_b_7 = ref(0);
let rot_channel = ref(0);   // high nibble of config.channels
let btn_channel = ref(0);   // low nibble of config.channels
let rot_zero_point = ref(0);
let rot_delta_14 = ref(0);
let rot_delta_7 = ref(0);
let rot_current_value_14 = ref(0);
let rot_current_value_7 = ref(0);
let firmware_version = ref(0);

function get_btn_channel(config) {
    return (config.channels >> 4) & 0xf;
}

function get_rot_channel(config) {
    return config.channels & 0xf;
}

// get a config object from the current settings

function get_current_config() {

    let config = {};

    config.config_version = config_version.value;
    config.rot_control_msb = rot_control_msb.value;
    config.rot_control_lsb = rot_control_lsb.value;
    config.btn_control_msb = btn_control_msb.value;
    config.btn_control_lsb = btn_control_lsb.value;
    config.btn_value_a_14 = btn_value_a_14.value;
    config.btn_value_b_14 = btn_value_b_14.value;
    config.btn_value_a_7 = btn_value_a_7.value;
    config.btn_value_b_7 = btn_value_b_7.value;

    config.channels = (rot_channel.value & 0xf) | ((btn_channel.value & 0xf) << 4);

    config.rot_zero_point = rot_zero_point.value;
    config.rot_delta_14 = rot_delta_14.value;
    config.rot_delta_7 = rot_delta_7.value;
    config.rot_current_value_14 = rot_current_value_14.value;
    config.rot_current_value_7 = rot_current_value_7.value;
    config.firmware_version = firmware_version.value;

    let flags = 0;
    flags |= rotate_extended.value ? midi.flags.cf_rotate_extended : 0;
    flags |= rotate_relative.value ? midi.flags.cf_rotate_relative : 0;
    flags |= led_invert.value ? midi.flags.cf_led_invert : 0;
    flags |= led_flash_on_rot.value ? midi.flags.cf_led_flash_on_rot : 0;
    flags |= led_flash_on_limit.value ? midi.flags.cf_led_flash_on_limit : 0;
    flags |= btn_momentary.value ? midi.flags.cf_btn_momentary : 0;
    flags |= btn_extended.value ? midi.flags.cf_btn_extended : 0;
    flags |= led_flash_on_click.value ? midi.flags.cf_led_flash_on_click : 0;
    flags |= led_flash_on_release.value ? midi.flags.cf_led_flash_on_release : 0;
    flags |= led_track_button_toggle.value ? midi.flags.cf_led_track_button_toggle : 0;

    flags |= ((acceleration.value & 1) != 0) ? midi.flags.cf_acceleration_lsb : 0;
    flags |= ((acceleration.value & 2) != 0) ? midi.flags.cf_acceleration_msb : 0;

    flags |= toggle.value ? midi.flags.cf_toggle : 0;
    flags |= button_tracks_rotation.value ? midi.flags.cf_button_tracks_rotation : 0;

    config.flags = flags;

    return config;
}

// apply current settings from a config object

function on_new_config(config) {

    config_version.value = config.config_version;
    rot_control_msb.value = config.rot_control_msb;
    rot_control_lsb.value = config.rot_control_lsb;
    btn_control_msb.value = config.btn_control_msb;
    btn_control_lsb.value = config.btn_control_lsb;
    btn_value_a_14.value = config.btn_value_a_14;
    btn_value_b_14.value = config.btn_value_b_14;
    btn_value_a_7.value = config.btn_value_a_7;
    btn_value_b_7.value = config.btn_value_b_7;

    rot_channel.value = get_rot_channel(config);
    btn_channel.value = get_btn_channel(config);

    rot_zero_point.value = config.rot_zero_point;
    rot_delta_14.value = config.rot_delta_14;
    rot_delta_7.value = config.rot_delta_7;
    rot_current_value_14.value = config.rot_current_value_14;
    rot_current_value_7.value = config.rot_current_value_7;
    firmware_version.value = config.firmware_version;

    let flags = config.flags;

    rotate_extended.value = (flags & midi.flags.cf_rotate_extended) != 0;
    rotate_relative.value = (flags & midi.flags.cf_rotate_relative) != 0;
    led_invert.value = (flags & midi.flags.cf_led_invert) != 0;
    led_flash_on_rot.value = (flags & midi.flags.cf_led_flash_on_rot) != 0;
    led_flash_on_limit.value = (flags & midi.flags.cf_led_flash_on_limit) != 0;
    btn_momentary.value = (flags & midi.flags.cf_btn_momentary) != 0;
    btn_extended.value = (flags & midi.flags.cf_btn_extended) != 0;
    led_flash_on_click.value = (flags & midi.flags.cf_led_flash_on_click) != 0;
    led_flash_on_release.value = (flags & midi.flags.cf_led_flash_on_release) != 0;
    led_track_button_toggle.value = (flags & midi.flags.cf_led_track_button_toggle) != 0;
    toggle.value = (flags & midi.flags.cf_toggle) != 0;
    button_tracks_rotation.value = (flags & midi.flags.cf_button_tracks_rotation) != 0;

    acceleration.value = (((flags & midi.flags.cf_acceleration_lsb) != 0) ? 1 : 0) + (((flags & midi.flags.cf_acceleration_msb) != 0) ? 2 : 0);
}

// set config_changed if current settings are different from last loaded/saved
// so the 'store to device' button can highlight if necessary

function diff() {
    config_changed.value = !shallowEqual(get_current_config(), stored_config);
};

function rot_limit() {
    return rotate_extended.value ? (1 << 14) : (1 << 7);
}

function constrain(a, min, max) {
    return Math.max(min, Math.min(a, max));
}

// and then loads of watchers for limits etc

function do_watch(x) {
    console.log(that.proxy.$refs);
}

do_watch(rot_zero_point);

//watch(rot_zero_point, (n) => { rot_zero_point.value = constrain(n, 0, rot_limit()); diff(); });
watch(rot_delta_14, (n) => { rot_delta_14.value = constrain(n, 1, 0x3fff); diff(); });
watch(btn_value_a_14, (n) => { btn_value_a_14.value = constrain(n, 0, 0x3fff); diff(); });
watch(btn_value_b_14, (n) => { btn_value_b_14.value = constrain(n, 0, 0x3fff); diff(); });
watch(rot_delta_7, (n) => { rot_delta_7.value = constrain(n, 1, 0x7f); diff(); });
watch(btn_value_a_7, (n) => { btn_value_a_7.value = constrain(n, 0, 0x7f); diff(); });
watch(btn_value_b_7, (n) => { btn_value_b_7.value = constrain(n, 0, 0x7f); diff(); });
watch(rot_channel, (n) => { rot_channel.value = constrain(n, 0, 15); diff(); });
watch(btn_channel, (n) => { btn_channel.value = constrain(n, 0, 15); diff(); });
watch(rotate_extended, (n) => { rotate_extended.value = n; diff(); });
watch(rotate_relative, (n) => { rotate_relative.value = n; diff(); });
watch(led_invert, (n) => { led_invert.value = n; diff(); });
watch(led_flash_on_rot, (n) => { led_flash_on_rot.value = n; diff(); });
watch(led_flash_on_limit, (n) => { led_flash_on_limit.value = n; diff(); });
watch(btn_momentary, (n) => { btn_momentary.value = n; diff(); });
watch(btn_extended, (n) => { btn_extended.value = n; diff(); });
watch(led_flash_on_click, (n) => { led_flash_on_click.value = n; diff(); });
watch(led_flash_on_release, (n) => { led_flash_on_release.value = n; diff(); });
watch(led_track_button_toggle, (n) => { led_track_button_toggle.value = n; diff(); });
watch(toggle, (n) => { toggle.value = n; diff(); });
watch(button_tracks_rotation, (n) => { button_tracks_rotation.value = n; diff(); });

function store_config() {
    props.device.config = get_current_config();
    midi.write_flash(props.device.device_index);
    config_changed.value = false;
}

// midi says a config was loaded from the device

midi.on_config_loaded((device) => {

    props.device.config = device.config;
    stored_config = Object.assign({}, toRaw(device.config));
    on_new_config(props.device.config);
    config_changed.value = false;
    that.proxy.$forceUpdate();
});

// midi says a config was saved to the device 

midi.on_config_saved((device) => {
    config_changed.value = false;
    stored_config = Object.assign({}, toRaw(device.config));
});

// preview knob

let default_matrix = rotation_matrix(50, 50, -150);

const rot_matrix = ref(default_matrix);

function rotation_matrix(cx, cy, angle) {
    let a = angle * 3.14159265 / 180;
    let c = Math.cos(a);
    let s = Math.sin(a);
    return `matrix(${c}, ${s}, ${-s}, ${c}, ${cx * (1 - c) + cy * s}, ${cy * (1 - c) - cx * s})`;
}

function is_LSB(cc) {
    return 32 <= cc && cc < 64;
}

function is_rot_extended(config) {
    return (config.flags & midi.flags.cf_rotate_extended) != 0;
}

function is_rot_relative(config) {
    return (config.flags & midi.flags.cf_rotate_relative) != 0;
}


let rotation_value = 0;

// midi says a CC value was sent
// if channel/cc the same as stored config rot then twist knob
// if channel/cc the same as stored config btn then animate button value

midi.on_control_change((channel, cc, val) => {

    let update_knob = false;

    if (channel == get_rot_channel(stored_config)) {

        // if (!is_rot_extended(stored_config)) {

        // } else {

        //     if (is_LSB(cc) && cc == stored_config.rot_control_lsb) {

        //         rotation_value = (rotation_value & 0x3f80) | val;

        //     } else if (cc == stored_config.rot_control_msb) {

        //         rotation_value = (rotation_value & 0x7f) | (val << 7);
        //     }

        // }

        if (is_LSB(cc) && is_rot_extended(stored_config) && cc == stored_config.rot_control_lsb) {
            if (is_rot_relative(stored_config)) {
                let amount = stored.rot_delta_7
                rotation_value = (rotation_value & 0x3f80) | val;
            } else {
                rotation_value = (rotation_value & 0x3f80) | val;
            }
            update_knob = true;
        } else if (cc == stored_config.rot_control_msb) {
            if ((stored_config.flags & midi.flags.cf_rotate_relative) != 0) {
                rotation_value = (rotation_value & 0x3f80) | val;
            } else {
                rotation_value = (rotation_value & 0x7f) | (val << 7);
            }
            update_knob = true;
        }
        if (update_knob) {
            let lower = -150;
            let upper = 150;
            let range = upper - lower;
            let angle = lower + (rotation_value / 16383.0) * range;
            rot_matrix.value = rotation_matrix(50, 50, angle);
        }
    }
});

// this doesn't work - port.close() does nothing

function toggle_connection(device) {
    if (!device.active) {
        midi.toggle_device_connection(device.device_index)
        config_changed.value = false;
    } else {
        disconnectModal.value = true;
    }
}

let connect_on_discovery = true;

if (connect_on_discovery) {
    midi.connect_device(props.device);
}

</script>

<template>
    <div class="container border rounded-3 py-3 bg-device border-secondary bg-secondary-subtle">
        <div class='row p-1'>

            <!-- Name, Serial, Buttons -->

            <div class='col-lg-3'>
                <div class='row'>
                    <div class='col-6'>
                        <div class="container">
                            <h5>{{ device.name }}</h5>
                        </div>
                    </div>
                </div>
                <div class='row mt-1'>
                    <div class="col-6 text-center mt-1">
                        <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                            @click='toggle_connection(device)'>
                            {{ !device.active ? 'Connect' : 'Disconnect' }}
                        </button>
                        <div class="small mt-3" v-if="device.active">
                            Firmware
                            <span class="text-primary-emphasis font-monospace">
                                v{{ firmware_version >> 8 }}.{{ (firmware_version &
                                0x7f).toString(10).padStart(2, '0') }}
                            </span>
                        </div>
                        <div class="small" v-if="device.active">
                            Serial #
                            <span class="text-body-secondary font-monospace">{{ device.serial_str }}</span>
                        </div>
                        <div class="row text-center mt-3" v-if="device.active">
                            <div class="col">
                                <svg viewBox="0 0 100 100" width="40" height="40">
                                    <ellipse id="preview-knob-outline" ry="45" rx="45" cy="50" cx="50" />
                                    <path id="preview-knob-tick" :transform="rot_matrix" d="M 50 40 V 20" />
                                </svg>
                            </div>
                        </div>
                        <div class="row text-center" v-if="device.active">
                            <div class="col">
                                <span style="width:45px;display:inline-block">
                                    <div class="progress" role="progressbar" aria-valuemax="16383" aria-valuemin="0"
                                        aria-valuenow="50" style="height:6px">
                                        <div class="progress-bar" :style="`width:${rotation_value * 100 / 16383}%`">
                                        </div>
                                    </div>
                                </span>
                            </div>
                        </div>
                    </div>
                    <div class="col-6">
                        <div class='btn-group-vertical' role="group" v-if='device.active'>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                @click='midi.flash_device_led(device.device_index)'>
                                <span class="mx-2">Flash LED</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                @click='midi.read_flash(device.device_index)'>
                                <span class="mx-2">Read from device</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                :class="{ 'red-text': config_changed }" @click='store_config()'>
                                <span class="mx-2">Store to device</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                v-if='device.active || 1'
                                @click="fileDownload(midi.get_config_json(device.device_index), 'midi_knob_settings.json');">
                                <span class="mx-2">Export config</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                @click="importModal = true">
                                <span class="mx-2">Import config</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                @click='flashModal = true'>
                                <span class="mx-2">Advanced</span>
                            </button>

                        </div>
                    </div>
                </div>
            </div>

            <!-- Rotation -->

            <div v-if='device.active' class='col-lg-3 mx-3 bg-body border border-secondary rounded'>
                <div class="row pt-2">
                    <div class='col mb-1'>
                        <strong>Rotation</strong>
                    </div>
                </div>
                <div class="row">
                    <div class='col-lg'>
                        <div class="row px-2">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none" for="extended_check_rot">Extended
                                        CC</label>
                                    <input class="form-check-input pull-left" type="checkbox" id="extended_check_rot"
                                        v-model="rotate_extended">
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class='col'>
                                <CCDropDown v-model="rot_control_msb" :label="rotate_extended ?
                                'MSB' : 'CC'
                                " />
                            </div>
                        </div>
                        <div class="row">
                            <div class='col'>
                                <CCDropDown v-model="rot_control_lsb" label="LSB" :hidden="!rotate_extended" />
                            </div>
                        </div>
                    </div>
                    <div class="col-lg">
                        <div class="row">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="rot_channel">
                                    <span class="input-group-text user-select-none">Chan</span>
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col" v-if="rotate_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="rot_delta_14">
                                    <span class="input-group-text user-select-none">Delta</span>
                                </div>
                            </div>
                            <div class="col" v-if="!rotate_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="rot_delta_7">
                                    <span class="input-group-text user-select-none">Delta</span>
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col">
                                <div class="input-group">
                                    <select class="form-select smaller-text" id="inputGroupSelect01"
                                        v-model="acceleration">
                                        <option selected value="0">Off</option>
                                        <option value="1">Low</option>
                                        <option value="2">Medium</option>
                                        <option value="3">High</option>
                                    </select>
                                    <label class="input-group-text" for="inputGroupSelect01">Accel</label>
                                </div>
                            </div>
                        </div>
                        <div class="row mx-4 mt-1 mb-0">
                            <div class="col mb-1" style="height:1.5rem;margin: 0px;padding: 0px;">
                                <Toggle v-model="rotate_relative" :checked-text-color='"var(--bs-btn-color)"'
                                    :unchecked-text-color='"var(--bs-btn-color)"'
                                    :checked-background-color='"var(--bs-tertiary-bg)"'
                                    :unchecked-background-color='"var(--bs-tertiary-bg)"'
                                    :border-color='"var(--bs-border-color)"'
                                    :unchecked-pill-color='"var(--bs-border-color)"'
                                    :checked-pill-color='"var(--bs-border-color)"' :checked-text='"Relative"'
                                    :unchecked-text='"Absolute"' />
                            </div>
                        </div>
                        <div class="row" :class="{ 'hide': !rotate_relative }">
                            <div class='col'>
                                <div class="input-group mb-2">
                                    <input type="number" class="form-control" v-model.number="rot_zero_point">
                                    <span class="input-group-text user-select-none">Zero</span>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Button -->

            <div v-if='device.active' class='col-lg-3 mx-3 bg-body border border-secondary rounded'>
                <div class="row pt-2">
                    <div class="col mb-1">
                        <strong>Button</strong>
                    </div>
                </div>
                <div class="row">
                    <div class="col-lg">
                        <div class="row">
                            <div class="col">
                                <div class="input-group">
                                    <input type="number" class="form-control" v-model.number="btn_channel">
                                    <span class="input-group-text user-select-none">Chan</span>
                                </div>
                            </div>
                        </div>
                        <div class="row mx-4 mt-1">
                            <div class="col mb-1" style="height:1.5rem;padding: 0px;">
                                <Toggle v-model="btn_momentary" :checked-text-color='"var(--bs-btn-color)"'
                                    :unchecked-text-color='"var(--bs-btn-color)"'
                                    :checked-background-color='"var(--bs-tertiary-bg)"'
                                    :unchecked-background-color='"var(--bs-tertiary-bg)"'
                                    :border-color='"var(--bs-border-color)"'
                                    :unchecked-pill-color='"var(--bs-border-color)"'
                                    :checked-pill-color='"var(--bs-border-color)"' :checked-text='"Momentary"'
                                    :unchecked-text='"Toggle"' />
                            </div>
                        </div>
                        <div class="row">
                            <div class="col" v-if="btn_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="btn_value_a_14">
                                    <span class="input-group-text user-select-none">
                                        {{ btn_momentary.value ? "ON" : "A" }}
                                    </span>
                                </div>
                            </div>
                            <div class="col" v-if="!btn_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="btn_value_a_7">
                                    <span class="input-group-text user-select-none">
                                        {{ btn_momentary.value ? "ON" : "A" }}
                                    </span>
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col" v-if="btn_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="btn_value_b_14">
                                    <span class="input-group-text user-select-none">
                                        {{ btn_momentary.value ? "OFF" : "B" }}
                                    </span>
                                </div>
                            </div>
                            <div class="col" v-if="!btn_extended">
                                <div class="input-group mb-2">
                                    <input type="number" class="form-control" v-model.number="btn_value_b_7">
                                    <span class="input-group-text user-select-none">
                                        {{ btn_momentary.value ? "OFF" : "B" }}
                                    </span>
                                </div>
                            </div>
                        </div>
                    </div>
                    <div class="col-lg">
                        <div class="row px-2">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none" for="extended_check_btn">Extended
                                        CC</label>
                                    <input class="form-check-input pull-left" type="checkbox" id="extended_check_btn"
                                        v-model="btn_extended">
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col">
                                <CCDropDown v-model="btn_control_msb" :label="btn_extended.value ? 'MSB' : 'CC'" />
                            </div>
                        </div>
                        <div class="row">
                            <div class="col mb-2">
                                <CCDropDown v-model="btn_control_lsb" label="LSB" :hidden="!btn_extended" />
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- LED -->

            <div v-if='device.active' class='col-lg-2 mx-3 bg-body border border-secondary rounded'>
                <div class="row pt-2">
                    <div class="col mb-1">
                        <strong>LED</strong>
                    </div>
                </div>
                <div class="row">
                    <div class="col">
                        <div class="row">
                            <div class="col">
                                Flash LED when:
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_rot">Knob is
                                    rotated</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_rot"
                                    v-model="led_flash_on_rot">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_limit">Rotation
                                    value
                                    hits
                                    limit</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_limit"
                                    v-model="led_flash_on_limit">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_click">Button is
                                    pressed</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_click"
                                    v-model="led_flash_on_click">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_release">Button
                                    is
                                    released</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_release"
                                    v-model="led_flash_on_release">
                            </div>
                        </div>
                    </div>
                    <hr class="my-1 mt-1" />
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="track_button_value">LED
                                    tracks
                                    button
                                    value</label>
                                <input class="form-check-input pull-left" type="checkbox" id="track_button_value"
                                    v-model="led_track_button_toggle">
                            </div>
                        </div>
                    </div>
                    <div class="row mb-2">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="led_invert">Invert
                                    LED</label>
                                <input class="form-check-input pull-left" type="checkbox" id="led_invert"
                                    v-model="led_invert">
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <!-- Modals -->

    <Modal v-model="flashModal" maxwidth="25%" closeable header="Advanced Functions">
        <div class="row mx-2 my-1">
            <div class="col">
                <p class="text-center red-text">Warning! Only click this if you
                    know what you're doing</p>
                <p class="text-center">
                    Instructions for firmware updating are
                    <a href='https://skilbeck.com/tiny-usb-midi-knob' target="_blank" rel="noreferrer noopener">
                        available here</a>.
                </p>
                <p class="text-center">
                    To get back to normal mode, unplug and plug it back in.
                </p>
            </div>
        </div>
        <div class="row mx-2 my-2">
            <div class="col text-center">
                <button class='btn btn-sm btn-danger' @click='midi.flash_mode(device.device_index); flashModal = false'>
                    Put device in Firmware Update Mode
                </button>
            </div>
        </div>
    </Modal>

    <Modal v-model="importModal" maxwidth="20%" closeable header="Import Settings">
        <div class="row mx-2 my-2">
            <div class="col mb-2 text-center">
                <input class="btn btn-sm border" type="file" id="import_settings" />
            </div>
        </div>
        <div class="row mx-2 my-3">
            <div class="col text-center">
                <button class="btn btn-sm btn-primary">
                    Import
                </button>
            </div>
        </div>
        <div class="row mx-2 my-3">
            <div class="col text-center">
                This is not yet implemented, sorry
            </div>
        </div>
    </Modal>

    <Modal v-model="disconnectModal" maxwidth="25%" closeable header="Disconnect">
        <div class="row mx-2">
            <div class="col text-center">
                Sorry, I can't seem to make disconnect work. You have to close the tab to disconnect all
                devices.
            </div>
        </div>
        <div class="row mt-4 mb-2">
            <div class="col text-center">
                <button class="btn btn-primary btn-sm" @click="disconnectModal = false">
                    Bummer
                </button>
            </div>
        </div>
    </Modal>
</template>

<style>
.hide {
    visibility: hidden;
}

.smaller-text {
    font-size: smaller;
}


[data-bs-theme='light'] .red-text {
    color: #B05800;
    font-weight: 600;
}

[data-bs-theme='dark'] .red-text {
    color: #D08010;
    font-weight: 600;
}

.slim-button {
    padding-top: 0px !important;
    padding-bottom: 2px !important;
}

.bg-device {
    background-color: var(--bs-body-bg);
}

#preview-knob-outline {
    stroke: var(--bs-secondary-border-subtle);
    fill: var(--bs-body-bg);
    stroke-width: 5;
}

#preview-knob-tick {
    stroke: var(--bs-secondary-color);
    stroke-width: 10;
    stroke-linecap: round;
}

.progress-bar {
    transition-duration: 0s;
}
</style>
