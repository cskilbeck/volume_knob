<script setup>

//////////////////////////////////////////////////////////////////////

import { toRaw, watch, ref, nextTick } from 'vue';

import Modal from './Modal.vue'
import HIDDropDown from './HIDDropDown.vue';

import hid from '../hid.js'

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

//////////////////////////////////////////////////////////////////////
// this is the config on the device to compare against

let stored_config = null;

//////////////////////////////////////////////////////////////////////

let ui_object = ui_from_config(hid.default_config);

let ui = ref(ui_object);

//////////////////////////////////////////////////////////////////////
// noddy compare for config objects

function shallowEqual(object1, object2) {

    if (object1 == null || object2 == null) {
        return false;
    }
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
// get a config object from the current ui
// could automate this somewhat...
// especially the flags

function config_from_ui() {

    return {
        config_version: ui.value.config_version,

        key_clockwise: ui.value.key_clockwise,
        key_counterclockwise: ui.value.key_counterclockwise,
        key_press: ui.value.key_press,

        mod_clockwise: ui.value.mod_clockwise,
        mod_counterclockwise: ui.value.mod_counterclockwise,
        mod_press: ui.value.mod_press,

        flags: (ui.value.cf_led_flash_on_cw ? hid.flags.cf_led_flash_on_cw : 0)
            | (ui.value.cf_led_flash_on_ccw ? hid.flags.cf_led_flash_on_ccw : 0)
            | (ui.value.cf_led_flash_on_press ? hid.flags.cf_led_flash_on_press : 0)
            | (ui.value.cf_led_flash_on_release ? hid.flags.cf_led_flash_on_release : 0)
            | (ui.value.cf_reverse_rotation ? hid.flags.cf_reverse_rotation : 0)
    };
}

//////////////////////////////////////////////////////////////////////
// get a ui object from a config object

function ui_from_config(config) {

    let errors = [];

    for (const field in config) {
        if (hid.default_config[field] == undefined) {
            errors.push(`Unknown field: ${field}`);
        }
    }

    for (const field in hid.default_config) {

        if (config[field] == undefined) {
            errors.push(`Missing field: ${field}`);
        } else {
            const cfg_type = typeof config[field];
            const default_type = typeof hid.default_config[field];
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

        key_clockwise: config.key_clockwise,
        key_counterclockwise: config.key_counterclockwise,
        key_press: config.key_press,

        mod_clockwise: config.mod_clockwise,
        mod_counterclockwise: config.mod_counterclockwise,
        mod_press: config.mod_press,

        cf_led_flash_on_cw: (config.flags & hid.flags.cf_led_flash_on_cw) != 0,
        cf_led_flash_on_ccw: (config.flags & hid.flags.cf_led_flash_on_ccw) != 0,
        cf_led_flash_on_press: (config.flags & hid.flags.cf_led_flash_on_press) != 0,
        cf_led_flash_on_release: (config.flags & hid.flags.cf_led_flash_on_release) != 0,
        cf_reverse_rotation: (config.flags & hid.flags.cf_reverse_rotation) != 0
    };
}

//////////////////////////////////////////////////////////////////////
// check for changes to ui, apply limits and check if the result is
// different from the last stored/loaded config on the device

function constrain(a, min, max) {
    return Math.max(min, Math.min(a, max));
}

watch(() => { return ui },
    (ui) => {

        // set config_changed if current settings are different from last loaded/saved
        // so the 'store to device' button can highlight if necessary

        config_changed.value = !shallowEqual(config_from_ui(), stored_config);
    },
    { deep: true }
);

//////////////////////////////////////////////////////////////////////

let collapsed = ref(false);

function toggle_expand() {
    collapsed.value = !collapsed.value;
}

//////////////////////////////////////////////////////////////////////
// hid says a config was loaded from the device - apply it to the ui

props.device.on_config_loaded = () => {
    stored_config = Object.assign({}, toRaw(props.device.config));
    let new_ui = ui_from_config(props.device.config);
    if (!new_ui) {
        console.log("No config, using default");
        Object.assign(new_ui, hid.default_config);
    }
    ui.value = new_ui;
    nextTick(() => {
        config_changed.value = false;
    });
};

//////////////////////////////////////////////////////////////////////
// hid says a config was saved to the device
// in theory there's a little race in here but in practice not a problem

props.device.on_config_saved = () => {
    console.log("Save config");
    stored_config = Object.assign({}, toRaw(props.device.config));
    config_changed.value = false;
};

//////////////////////////////////////////////////////////////////////
// store current UI to the device
// update stored_config when we get the ack that it was written

function store_config() {
    console.log("Store config");
    props.device.config = config_from_ui();
    hid.set_config(props.device);
}

//////////////////////////////////////////////////////////////////////

function json_from_config(config) {
    return JSON.stringify(config, " ", 4);
}

//////////////////////////////////////////////////////////////////////

function reset_to_defaults() {

    console.log("reset_to_defaults");
    Object.assign(props.device.config, hid.default_config);
    ui.value = ui_from_config(props.device.config);
}

//////////////////////////////////////////////////////////////////////

function copy_config() {
    const config = config_from_ui();
    const json = json_from_config(config);
    navigator.clipboard.writeText(json);
}

//////////////////////////////////////////////////////////////////////

function show_paste_dialog() {
    config_paste_textarea_contents.value = "";
    pasteConfigModal.value = true;
}

//////////////////////////////////////////////////////////////////////

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

function rotation_matrix(cx, cy, angle) {
    let a = angle * 3.14159265 / 180;
    let c = Math.cos(a);
    let s = Math.sin(a);
    return `matrix(${c}, ${s}, ${- s}, ${c}, ${cx * (1 - c) + cy * s}, ${cy * (1 - c) - cx * s})`;
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
                <div class='row'>
                    <div class="col-6 text-center">
                        <div class="small" v-if="device.active">
                            Firmware version
                            <div class="text-body-secondary font-monospace">
                                {{ props.device.firmware_version_str }}
                            </div>
                        </div>
                    </div>
                    <div class="col-6">
                        <div class='btn-group-vertical' role="group" v-if='device.active'>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle' @click='hid.flash_device_led(device)'>
                                <span class="mx-2">Flash LED</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle' @click='hid.get_config(device)'>
                                <span class="mx-2">Read from device</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle' :class="{ 'red-text': config_changed }" @click='store_config()'>
                                <span class="mx-2">Store to device</span>
                            </button>

                            <div class="dropdown w-100">
                                <button class="w-100 btn btn-sm rounded-0 tertiary-bg border-secondary-subtle dropdown-toggle border-top-0" href="#" role="button" data-bs-toggle="dropdown"
                                    aria-expanded="false">
                                    Settings
                                </button>
                                <ul class="dropdown-menu">
                                    <li>
                                        <a class="dropdown-item" href="#" @click="fileDownload(json_from_config(config_from_ui()), 'tiny_volume_knob_settings.json');">
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
                                    <li>
                                        <hr class='m-0' />
                                    </li>
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
                    <div class='col-8'>
                        <strong>Rotation</strong>
                    </div>
                    <div class='col-3'>
                        <div class="form-check pb-0 pt-1 m-0">
                            <input class="form-check-input" type="checkbox" id="reverse_rotation" v-model="ui.cf_reverse_rotation">
                            <label class="form-check-label user-select-none" for="reverse_rotation">
                                Reversed
                            </label>
                        </div>
                    </div>
                </div>
                <div class="row mb-3">
                    <div class='col-lg'>
                        <div class="row">
                            <div class='col mx-3'>
                                <HIDDropDown v-model:keycode="ui.key_clockwise" v-model:mod="ui.mod_clockwise">
                                    Clockwise
                                </HIDDropDown>
                            </div>
                        </div>
                        <div class="row">
                            <div class='col mx-3'>
                                <HIDDropDown v-model:keycode="ui.key_counterclockwise" v-model:mod="ui.mod_counterclockwise">
                                    Counter-clockwise
                                </HIDDropDown>
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
                    <div class="col mx-3">
                        <HIDDropDown v-model:keycode="ui.key_press" v-model:mod="ui.mod_press">Key</HIDDropDown>
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
                                    Rotate clockwise
                                </label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_rot" v-model="ui.cf_led_flash_on_cw">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_limit">
                                    Rotate counter-clockwise
                                </label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_limit" v-model="ui.cf_led_flash_on_ccw">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_click">
                                    Press
                                </label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_click" v-model="ui.cf_led_flash_on_press">
                            </div>
                        </div>
                    </div>
                    <div class="row mb-2">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_release">
                                    Release
                                </label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_release" v-model="ui.cf_led_flash_on_release">
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
                <button class='btn btn-sm btn-danger' @click='hid.goto_firmware_update_mode(device); flashModal = false'>
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
