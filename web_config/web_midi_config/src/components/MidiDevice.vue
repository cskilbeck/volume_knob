<script setup>

//////////////////////////////////////////////////////////////////////

import { toRaw, watch, ref, nextTick } from 'vue';

import Toggle from './Toggle.vue'
import Modal from './Modal.vue'
import CCDropDown from './MidiCCDropDown.vue';

import CC from '../CC.js'
import midi from '../Midi.js'

import fileDownload from 'js-file-download';

//////////////////////////////////////////////////////////////////////

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

//////////////////////////////////////////////////////////////////////
// 3 modal dialogs

const flashModal = ref(false);
const disconnectModal = ref(false);
const pasteConfigModal = ref(false);
const errorModal = ref(false);

let config_paste_textarea_contents = ref("");

let error_messages = ref([]);

//////////////////////////////////////////////////////////////////////

function cookie_name() {
    return `${props.device.name}_label`;
}

let device_label = ref(localStorage.getItem(cookie_name()) || "Unnamed");

function save_name() {
    if (device_label.value.length == 0) {
        device_label.value = "Unnamed";
    }
    localStorage.setItem(cookie_name(), device_label.value, 400);
}

//////////////////////////////////////////////////////////////////////
// is the current config different from what's on the device?

let config_changed = ref(false);

// if loading from device, suppress some computed things (MSB/LSB/etc stuff)

let loading_config = false;

//////////////////////////////////////////////////////////////////////
// this is the config on the device to compare against

let stored_config = null;

//////////////////////////////////////////////////////////////////////

let ui_object = ui_from_config(midi.default_config);

let ui = ref(ui_object);

//////////////////////////////////////////////////////////////////////
// noddy compare for config objects

function shallowEqual(object1, object2) {

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

//////////////////////////////////////////////////////////////////////

function get_channels(rot, btn) {
    return (rot & 0xf) | ((btn & 0xf) << 4);
}

function get_btn_channel(config) {
    return (config.channels >> 4) & 0xf;
}

function get_rot_channel(config) {
    console.log(config);
    return config.channels & 0xf;
}

//////////////////////////////////////////////////////////////////////
// get a config object from the current ui
// could automate this somewhat...
// especially the flags

function config_from_ui() {

    return {
        config_version: ui.value.config_version,
        rot_control_msb: ui.value.rot_control_msb,
        rot_control_lsb: ui.value.rot_control_lsb,
        btn_control_msb: ui.value.btn_control_msb,
        btn_control_lsb: ui.value.btn_control_lsb,
        btn_value_a_14: ui.value.btn_value_a_14,
        btn_value_b_14: ui.value.btn_value_b_14,
        btn_value_a_7: ui.value.btn_value_a_7,
        btn_value_b_7: ui.value.btn_value_b_7,
        channels: get_channels(ui.value.rot_channel, ui.value.btn_channel),
        rot_zero_point: ui.value.rot_zero_point,
        rot_delta_14: ui.value.rot_delta_14,
        rot_delta_7: ui.value.rot_delta_7,
        rot_current_value_14: ui.value.rot_current_value_14,
        rot_current_value_7: ui.value.rot_current_value_7,
        acceleration: ui.value.acceleration,

        flags: (ui.value.rotate_extended ? midi.flags.cf_rotate_extended : 0)
            | (ui.value.rotate_relative ? midi.flags.cf_rotate_relative : 0)
            | (ui.value.led_invert ? midi.flags.cf_led_invert : 0)
            | (ui.value.led_flash_on_rot ? midi.flags.cf_led_flash_on_rot : 0)
            | (ui.value.led_flash_on_limit ? midi.flags.cf_led_flash_on_limit : 0)
            | (ui.value.btn_momentary ? midi.flags.cf_btn_momentary : 0)
            | (ui.value.btn_extended ? midi.flags.cf_btn_extended : 0)
            | (ui.value.led_flash_on_click ? midi.flags.cf_led_flash_on_click : 0)
            | (ui.value.led_flash_on_release ? midi.flags.cf_led_flash_on_release : 0)
            | (ui.value.led_track_button_toggle ? midi.flags.cf_led_track_button_toggle : 0)
            | (ui.value.toggle ? midi.flags.cf_toggle : 0)
            | (ui.value.button_tracks_rotation ? midi.flags.cf_button_tracks_rotation : 0)
            | (ui.value.rotate_reverse ? midi.flags.cf_rotate_reverse : 0)
    };
}

//////////////////////////////////////////////////////////////////////
// get a ui object from a config object

function ui_from_config(config) {

    let errors = [];

    for (const field in config) {
        if (midi.default_config[field] == undefined) {
            errors.push(`Unknown field: ${field}`);
        }
    }

    for (const field in midi.default_config) {

        if (config[field] == undefined) {
            errors.push(`Missing field: ${field}`);
        } else {
            const cfg_type = typeof config[field];
            const default_type = typeof midi.default_config[field];
            if (cfg_type !== default_type) {
                errors.push(`Field [${field}] is ${cfg_type}, should be ${default_type}`);
            }
        }
    }

    if (errors.length != 0) {
        error_messages.value = errors;
        return null;
    }

    return {
        config_version: config.config_version,
        rot_control_msb: config.rot_control_msb,
        rot_control_lsb: config.rot_control_lsb,
        btn_control_msb: config.btn_control_msb,
        btn_control_lsb: config.btn_control_lsb,
        btn_value_a_14: config.btn_value_a_14,
        btn_value_b_14: config.btn_value_b_14,
        btn_value_a_7: config.btn_value_a_7,
        btn_value_b_7: config.btn_value_b_7,
        rot_channel: get_rot_channel(config),
        btn_channel: get_btn_channel(config),
        rot_zero_point: config.rot_zero_point,
        rot_delta_14: config.rot_delta_14,
        rot_delta_7: config.rot_delta_7,
        rot_current_value_14: config.rot_current_value_14,
        rot_current_value_7: config.rot_current_value_7,
        acceleration: config.acceleration,

        rotate_extended: (config.flags & midi.flags.cf_rotate_extended) != 0,
        rotate_relative: (config.flags & midi.flags.cf_rotate_relative) != 0,
        led_invert: (config.flags & midi.flags.cf_led_invert) != 0,
        led_flash_on_rot: (config.flags & midi.flags.cf_led_flash_on_rot) != 0,
        led_flash_on_limit: (config.flags & midi.flags.cf_led_flash_on_limit) != 0,
        btn_momentary: (config.flags & midi.flags.cf_btn_momentary) != 0,
        btn_extended: (config.flags & midi.flags.cf_btn_extended) != 0,
        led_flash_on_click: (config.flags & midi.flags.cf_led_flash_on_click) != 0,
        led_flash_on_release: (config.flags & midi.flags.cf_led_flash_on_release) != 0,
        led_track_button_toggle: (config.flags & midi.flags.cf_led_track_button_toggle) != 0,
        toggle: (config.flags & midi.flags.cf_toggle) != 0,
        button_tracks_rotation: (config.flags & midi.flags.cf_button_tracks_rotation) != 0,
        rotate_reverse: (config.flags & midi.flags.cf_rotate_reverse) != 0,
        button_tracks_rotation: (config.flags & midi.flags.cf_button_tracks_rotation) != 0
    };
}

props.device.on_control_change = (channel, cc, val) => {

    console.log(channel, cc, val);

    let update_knob = false;

    if (channel == get_rot_channel(stored_config)) {

        if (is_LSB(cc) && is_rot_extended(stored_config) && cc == stored_config.rot_control_lsb) {

            if (is_rot_relative(stored_config)) {

                rotation_value += (rotation_value << 7) & 0x3f80;

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
};

//////////////////////////////////////////////////////////////////////
// check for changes to ui, apply limits and check if the result is
// different from the last stored/loaded config on the device

function constrain(a, min, max) {
    return Math.max(min, Math.min(a, max));
}

watch(() => { return ui },
    (ui) => {
        ui.value.rot_zero_point = constrain(ui.value.rot_zero_point, 0, 128);
        ui.value.rot_delta_14 = constrain(ui.value.rot_delta_14, 1, 0x3fff);
        ui.value.btn_value_a_14 = constrain(ui.value.btn_value_a_14, 0, 0x3fff);
        ui.value.btn_value_b_14 = constrain(ui.value.btn_value_b_14, 0, 0x3fff);
        ui.value.rot_delta_7 = constrain(ui.value.rot_delta_7, 1, 0x7f);
        ui.value.btn_value_a_7 = constrain(ui.value.btn_value_a_7, 0, 0x7f);
        ui.value.btn_value_b_7 = constrain(ui.value.btn_value_b_7, 0, 0x7f);
        ui.value.rot_channel = constrain(ui.value.rot_channel, 0, 15);
        ui.value.btn_channel = constrain(ui.value.btn_channel, 0, 15);

        // set config_changed if current settings are different from last loaded/saved
        // so the 'store to device' button can highlight if necessary

        config_changed.value = !shallowEqual(config_from_ui(), stored_config);
    },
    { deep: true }
);

//////////////////////////////////////////////////////////////////////
// midi says a config was loaded from the device - apply it to the ui

props.device.on_config_loaded = () => {
    loading_config = true;
    stored_config = Object.assign({}, toRaw(props.device.config));
    console.log("CONFIG LOADED", stored_config);
    ui.value = ui_from_config(props.device.config) || midi.default_config;
    nextTick(() => {
        loading_config = false;
        config_changed.value = false;
    });
};

//////////////////////////////////////////////////////////////////////
// midi says a config was saved to the device
// in theory there's a little race in here but in practice not a problem

props.device.on_config_saved = () => {
    stored_config = Object.assign({}, toRaw(props.device.config));
    config_changed.value = false;
    console.log("CONFIG SAVED", stored_config);
};

//////////////////////////////////////////////////////////////////////
// store current UI to the device
// update stored_config when we get the ack that it was written

function store_config() {
    props.device.config = config_from_ui();
    console.log("CONFIG SAVING", props.device.config);
    midi.write_flash(props.device.device_index);
}

//////////////////////////////////////////////////////////////////////
// rotation/button CC MSB changed
// if it's an MSB, set extended flag and set LSB to the alt

// these two functions are very similar
// but refactoring them looks like more hassle than it's worth

// rot version

function cc_rot_msb_changed() {
    if (!loading_config) {
        let cc = CC.CCs[ui.value.rot_control_msb];
        let is_msb = CC.is_MSB(cc);
        ui.value.rotate_extended = is_msb;
        if (is_msb) {
            ui.value.rot_control_lsb = cc.alt;
        }
    }
}

// button version

function cc_btn_msb_changed() {
    if (!loading_config) {
        let cc = CC.CCs[ui.value.btn_control_msb];
        let is_msb = CC.is_MSB(cc);
        ui.value.btn_extended = is_msb;
        if (is_msb) {
            ui.value.btn_control_lsb = cc.alt;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// preview knob

let default_matrix = rotation_matrix(50, 50, -150);

const rot_matrix = ref(default_matrix);

function rotation_matrix(cx, cy, angle) {
    let a = angle * 3.14159265 / 180;
    let c = Math.cos(a);
    let s = Math.sin(a);
    return `matrix(${c}, ${s}, ${- s}, ${c}, ${cx * (1 - c) + cy * s}, ${cy * (1 - c) - cx * s})`;
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

//////////////////////////////////////////////////////////////////////
// this doesn't work - port.close() does nothing

function toggle_connection(device) {
    if (!device.active) {
        midi.toggle_device_connection(device.device_index)
        config_changed.value = false;
    } else {
        disconnectModal.value = true;
    }
}

//////////////////////////////////////////////////////////////////////
// read device config when it's discovered

let connect_on_discovery = true;

if (connect_on_discovery) {
    midi.connect_device(props.device);
}

let collapsed = ref(false);

function toggle_expand() {
    collapsed.value = !collapsed.value;
}

//////////////////////////////////////////////////////////////////////

function copy_config() {
    navigator.clipboard.writeText(JSON.stringify(config_from_ui(), 4, " "));
}

function show_paste_dialog() {
    config_paste_textarea_contents.value = "";
    pasteConfigModal.value = true;
}

function paste_config(json_text) {

    let new_config = {};

    try {
        new_config = JSON.parse(json_text);
    } catch (err) {
        error_messages.value = [err.message];
        errorModal.value = true;
        return;
    }

    let original_config = config_from_ui();
    let new_ui = ui_from_config(new_config);
    if (new_ui == null) {
        errorModal.value = true;
    } else {
        ui.value = new_ui;
    }
}

//////////////////////////////////////////////////////////////////////

function reset_to_defaults() {

    props.device.config = midi.default_config;
    ui.value = ui_from_config(midi.default_config);
}

//////////////////////////////////////////////////////////////////////

</script>

<template>

    <svg class='d-none'>
        <symbol id='little-arrow' xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="currentColor" viewBox="0 0 16 16">
            <path d="m12.14 8.753-5.482 4.796c-.646.566-1.658.106-1.658-.753V3.204a1 1 0 0 1 1.659-.753l5.48 4.796a1 1 0 0 1 0 1.506z" />
        </symbol>
    </svg>

    <div class="container border rounded-3 bg-device border-secondary bg-secondary-subtle pt-2 mb-4" :class="collapsed ? 'pb-2' : ' pb-4'">

        <div class='row'>
            <div class='col text-left ms-2' :class="!collapsed ? 'mb-1' : ''">
                <div class="row">
                    <div class="col ps-0">
                        <div class="row">
                            <div class="col pe-0 me-0">
                                <button class="btn" @click="toggle_expand()" style="--bs-btn-padding-y: .25rem; --bs-btn-padding-x: .5rem; --bs-btn-font-size: .75rem;">
                                    <svg width="20" height="20" style="transition:0.1s" :transform="rotation_matrix(0, 0, collapsed ? 0 : 90)">
                                        <use href="#little-arrow"></use>
                                    </svg>
                                </button>
                                <strong>{{ device.name }}</strong>
                                <span class="d-inline-block" style="width:1em"></span>
                                <input class="bg-secondary-subtle text-secondary rounded focus-ring ps-2 bright-focus-input" type="text" @blur="save_name()" v-model="device_label"
                                    @keypress='(e) => { e.key === "Enter" && e.currentTarget.blur(); }'>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <div class='row p-1' v-if="!collapsed">

            <!-- Name, Serial, Buttons -->

            <div class='col-lg-3'>
                <div class='row mt-1'>
                    <div class="col-6 text-center">
                        <button class='btn btn-sm tertiary-bg border border-secondary-subtle' @click='toggle_connection(device)'>
                            {{ !device.active ? 'Connect' : 'Disconnect' }}
                        </button>
                        <div class="small mt-3" v-if="device.active">
                            Firmware version
                            <div class="text-body-secondary font-monospace">
                                {{ props.device.firmware_version_str }}
                            </div>
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
                                <span style="width:40px;display:inline-block">
                                    <div class="progress border bg-body border-secondary" role="progressbar" aria-valuemax="16383" aria-valuemin="0" aria-valuenow="0" style="height:8px">
                                        <div class="progress-bar value-bar" :style="`width:${rotation_value * 100 / 16383}%`">
                                        </div>
                                    </div>
                                </span>
                            </div>
                        </div>
                    </div>
                    <div class="col-6">
                        <div class='btn-group-vertical' role="group" v-if='device.active'>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle' @click='midi.flash_device_led(device.device_index)'>
                                <span class="mx-2">Flash LED</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle' @click='midi.read_flash(device.device_index)'>
                                <span class="mx-2">Read from device</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle' :class="{ 'red-text': config_changed }" @click='store_config()'>
                                <span class="mx-2">Store to device</span>
                            </button>

                            <div class="dropdown w-100">
                                <button class="w-100 btn btn-sm rounded-0 tertiary-bg border-secondary-subtle btn-secondary dropdown-toggle border-top-0" href="#" role="button"
                                    data-bs-toggle="dropdown" aria-expanded="false">
                                    Settings
                                </button>
                                <ul class="dropdown-menu">
                                    <li>
                                        <a class="dropdown-item" href="#" @click="fileDownload(JSON.stringify(config_from_ui(), 4, ' '), 'midi_knob_settings.json');">
                                            Export
                                        </a>
                                    </li>
                                    <li>
                                        <a class="dropdown-item" href="#" @click="copy_config()">
                                            Copy
                                        </a>
                                    </li>
                                    <li>
                                        <a class="dropdown-item" href="#" @click='show_paste_dialog()'>
                                            Paste
                                        </a>
                                    </li>
                                    <hr class="my-0" />
                                    <li>
                                        <a class="dropdown-item" href="#" @click='reset_to_defaults()'>
                                            Reset to defaults
                                        </a>
                                    </li>
                                </ul>
                            </div>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle' @click='flashModal = true'>
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
                        <div class="row ps-1">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none" for="extended_check_rot">
                                        Extended CC
                                    </label>
                                    <input class="form-check-input pull-left" type="checkbox" id="extended_check_rot" v-model="ui.rotate_extended">
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class='col'>
                                <CCDropDown v-model="ui.rot_control_msb" v-on:update:modelValue="cc_rot_msb_changed">
                                    {{ ui.rotate_extended ? 'MSB' : 'CC' }}
                                </CCDropDown>
                            </div>
                        </div>
                        <div class="row">
                            <div class='col' :class="{ hide: !ui.rotate_extended }">
                                <CCDropDown v-model="ui.rot_control_lsb">
                                    LSB
                                </CCDropDown>
                            </div>
                        </div>
                        <div class="row ps-1">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none" for="rotate_reverse_check">
                                        Reverse rotation
                                    </label>
                                    <input class="form-check-input pull-left" type="checkbox" id="rotate_reverse_check" v-model="ui.rotate_reverse">
                                </div>
                            </div>
                        </div>
                    </div>
                    <div class="col-lg">
                        <div class="row">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="ui.rot_channel">
                                    <span class="input-group-text user-select-none">Chan</span>
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col" v-if="ui.rotate_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="ui.rot_delta_14">
                                    <span class="input-group-text user-select-none">Delta</span>
                                </div>
                            </div>
                            <div class="col" v-if="!ui.rotate_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="ui.rot_delta_7">
                                    <span class="input-group-text user-select-none">Delta</span>
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col">
                                <div class="input-group">
                                    <select class="form-select smaller-text" id="accel_input_group" v-model="ui.acceleration">
                                        <option selected value="0">Off</option>
                                        <option value="1">Low</option>
                                        <option value="2">Medium</option>
                                        <option value="3">High</option>
                                    </select>
                                    <label class="input-group-text" for="accel_input_group">Accel</label>
                                </div>
                            </div>
                        </div>
                        <div class="row mx-4 mt-1 mb-0">
                            <div class="col mb-1" style="height:1.5rem;margin: 0px;padding: 0px;">
                                <Toggle v-model="ui.rotate_relative" :checked-background-color='"var(--bs-tertiary-bg)"' :unchecked-background-color='"var(--bs-tertiary-bg)"'
                                    :border-color='"var(--bs-border-color)"' :unchecked-pill-color='"var(--bs-border-color)"' :checked-pill-color='"var(--bs-border-color)"'>
                                    <template #unchecked>
                                        Absolute
                                    </template>
                                    <template #checked>
                                        Relative
                                    </template>
                                </Toggle>
                            </div>
                        </div>
                        <div class="row" :class="{ 'hide': !ui.rotate_relative }">
                            <div class='col'>
                                <div class="input-group mb-2">
                                    <input type="number" class="form-control" v-model.number="ui.rot_zero_point">
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
                        <div class="row px-2">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none" for="extended_check_btn">Extended
                                        CC</label>
                                    <input class="form-check-input pull-left" type="checkbox" id="extended_check_btn" v-model="ui.btn_extended">
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col">
                                <CCDropDown v-model="ui.btn_control_msb" v-on:update:modelValue="cc_btn_msb_changed">
                                    {{ ui.btn_extended ? 'MSB' : 'CC' }}
                                </CCDropDown>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col mb-2" :class="{ hide: !ui.btn_extended }">
                                <CCDropDown v-model="ui.btn_control_lsb">
                                    LSB
                                </CCDropDown>
                            </div>
                        </div>
                    </div>
                    <div class="col-lg">
                        <div class="row">
                            <div class="col">
                                <div class="input-group">
                                    <input type="number" class="form-control" v-model.number="ui.btn_channel">
                                    <span class="input-group-text user-select-none">Chan</span>
                                </div>
                            </div>
                        </div>
                        <div class="row mx-4 mt-1">
                            <div class="col mb-1" style="height:1.5rem;padding: 0px;">
                                <Toggle v-model="ui.btn_momentary" :checked-background-color='"var(--bs-tertiary-bg)"' :unchecked-background-color='"var(--bs-tertiary-bg)"'
                                    :border-color='"var(--bs-border-color)"' :unchecked-pill-color='"var(--bs-border-color)"' :checked-pill-color='"var(--bs-border-color)"'>
                                    <template #checked>
                                        Momentary
                                    </template>
                                    <template #unchecked>
                                        Toggle
                                    </template>
                                </Toggle>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col" v-if="ui.btn_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="ui.btn_value_a_14">
                                    <span class="input-group-text user-select-none">
                                        {{ ui.btn_momentary ? "ON" : "A" }}
                                    </span>
                                </div>
                            </div>
                            <div class="col" v-if="!ui.btn_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="ui.btn_value_a_7">
                                    <span class="input-group-text user-select-none">
                                        {{ ui.btn_momentary ? "ON" : "A" }}
                                    </span>
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col" v-if="ui.btn_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="ui.btn_value_b_14">
                                    <span class="input-group-text user-select-none">
                                        {{ ui.btn_momentary ? "OFF" : "B" }}
                                    </span>
                                </div>
                            </div>
                            <div class="col" v-if="!ui.btn_extended">
                                <div class="input-group mb-2">
                                    <input type="number" class="form-control" v-model.number="ui.btn_value_b_7">
                                    <span class="input-group-text user-select-none">
                                        {{ ui.btn_momentary ? "OFF" : "B" }}
                                    </span>
                                </div>
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
                                Flash on:
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_rot">
                                    Rotate
                                </label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_rot" v-model="ui.led_flash_on_rot">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_limit">
                                    Limit
                                </label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_limit" v-model="ui.led_flash_on_limit">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_click">
                                    Press
                                </label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_click" v-model="ui.led_flash_on_click">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_release">
                                    Release
                                </label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_release" v-model="ui.led_flash_on_release">
                            </div>
                        </div>
                    </div>
                    <hr class="my-1 mt-1" />
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="track_button_value">
                                    Track button
                                </label>
                                <input class="form-check-input pull-left" type="checkbox" id="track_button_value" v-model="ui.led_track_button_toggle">
                            </div>
                        </div>
                    </div>
                    <div class="row mb-2">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="led_invert">
                                    Invert
                                </label>
                                <input class="form-check-input pull-left" type="checkbox" id="led_invert" v-model="ui.led_invert">
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
                <p class="text-center red-text">
                    Warning! Only click this if you know what you're doing</p>
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

    <Modal v-model="disconnectModal" maxwidth="25%" closeable header="Disconnect">
        <div class="row mx-2">
            <div class="col text-center">
                Sorry, I can't seem to make disconnect work.<br>
                You have to close the tab to disconnect all devices.
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

    <Modal v-model="pasteConfigModal" maxwidth="30%" closeable backdrop-no-close header="Paste Settings">
        <div class="row mx-2">
            <textarea class="font-monospace form-control smaller-text" style="height:26rem;" v-model="config_paste_textarea_contents" placeholder="Paste your settings JSON in here...">
        </textarea>
        </div>
        <div class="row mt-4 mb-2">
            <div class="col-8 ps-4">

            </div>
            <div class="col-4 text-right">
                <div class="row">
                    <div class="col">
                        <button class="btn btn-secondary btn-sm" @click='pasteConfigModal = false'>
                            Cancel
                        </button>
                    </div>
                    <div class="col">
                        <button class="btn btn-primary btn-sm" @click='pasteConfigModal = false; paste_config(config_paste_textarea_contents)'>
                            Apply
                        </button>
                    </div>
                </div>
            </div>
        </div>
    </Modal>

    <Modal v-model="errorModal" maxwidth="40%" closeable header='Errors'>
        <div class="row mx-2">
            <div class="col">
                <ul v-for="(err, idx) in error_messages" :key="idx" class="list-group list-group-flush">
                    <li class="list-group-item font-monospace" :class='(idx != error_messages.length - 1) ? "border-bottom" : ""'>
                        {{ err }}
                    </li>
                </ul>
            </div>
        </div>
        <div class="row mt-4 mb-2">
            <div class="col text-end pe-4">
                <div class="row">
                    <div class="col text-right">
                        <button class="btn btn-primary btn-sm" @click='errorModal = false'>
                            Close
                        </button>
                    </div>
                </div>
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

.btn.active,
.btn.show,
.btn:first-child:active,
:not(.btn-check)+.btn:active {
    border-color: transparent;
}

input.bright-focus-input:active,
input.bright-focus-input:focus {
    color: var(--bs-body-color) !important;
    border:
        var(--bs-border-width) var(--bs-border-style) color-mix(in srgb, var(--bs-primary) 50%, white) !important;
}

input.bright-focus-input,
input.bright-focus-input {
    border:
        var(--bs-border-width) solid transparent !important;
}

[data-bs-theme='light'] .value-bar {
    background-color: var(--bs-primary-bg-subtle);
}

[data-bs-theme='dark'] .value-bar {
    background-color: var(--bs-primary-border-subtle);
}
</style>
