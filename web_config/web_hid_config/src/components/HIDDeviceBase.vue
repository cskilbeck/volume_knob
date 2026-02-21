<script setup>

//////////////////////////////////////////////////////////////////////

import { useHIDDevice } from '../useHIDDevice.js'

import Modal from './Modal.vue'
import HIDDropDown from './HIDDropDown.vue'

import hid from '../hid.js'
import fileDownload from 'js-file-download'

//////////////////////////////////////////////////////////////////////

const props = defineProps({
    device: {
        type: Object,
        required: true
    },
    supports_mouse: {
        type: Boolean,
        default: false
    }
});

//////////////////////////////////////////////////////////////////////

const {
    flashModal,
    pasteConfigModal,
    errorModal,
    config_paste_textarea_contents,
    error_messages,
    device_label,
    save_name,
    config_changed,
    ui,
    collapsed,
    toggle_expand,
    store_config,
    json_from_config,
    config_from_ui,
    reset_to_defaults,
    copy_config,
    show_paste_dialog,
    paste_config,
    rotation_matrix,
} = useHIDDevice(props, { supports_mouse: props.supports_mouse });

//////////////////////////////////////////////////////////////////////

</script>

<template>

    <svg class='d-none'>
        <symbol id='little-arrow' xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="currentColor" viewBox="0 0 16 16">
            <path d="m12.14 8.753-5.482 4.796c-.646.566-1.658.106-1.658-.753V3.204a1 1 0 0 1 1.659-.753l5.48 4.796a1 1 0 0 1 0 1.506z" />
        </symbol>
    </svg>

    <div class="container border rounded-0 bg-device border-secondary bg-secondary-subtle pt-2 mb-4" :class="collapsed ? 'pb-2' : ' pb-4'" style="min-width: 1000px;">

        <div class='row py-1'>
            <div class='col text-left ms-2' :class="!collapsed ? 'mb-1' : ''">
                <div class="row">
                    <div class="col-5 ps-0">
                        <div class="row">
                            <div class="col pe-0 me-0">
                                <button class="btn" @click="toggle_expand()" style="--bs-btn-padding-y: .25rem; --bs-btn-padding-x: .5rem; --bs-btn-font-size: .75rem;">
                                    <svg width="20" height="20" style="transition:0.1s" :transform="rotation_matrix(0, 0, collapsed ? 0 : 90)">
                                        <use href="#little-arrow"></use>
                                    </svg>
                                </button>
                                <strong>{{ device.name }}</strong>
                                <span class="d-inline-block" style="width:1em"></span>
                                <input class="bg-secondary-subtle text-secondary rounded-0 focus-ring ps-2 bright-focus-input" type="text" @blur="save_name()" v-model="device_label"
                                    @keypress='(e) => { e.key === "Enter" && e.currentTarget.blur(); }'>
                            </div>
                        </div>
                    </div>
                    <div class="col-5">
                        <div class='btn-group rounded-0' role="group" v-if='device.active'>

                            <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle' @click='hid.flash_device_led(device)'>
                                Flash LED
                            </button>

                            <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle' @click='hid.get_config(device)'>
                                Read from device
                            </button>

                            <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle' :class="{ 'red-text': config_changed }" @click='store_config()'>
                                Store to device
                            </button>

                            <div class="btn-group rounded-0" role="group">
                                <button class="w-100 btn btn-sm rounded-0 tertiary-bg border-secondary-subtle dropdown-toggle" href="#" role="button" data-bs-toggle="dropdown" aria-expanded="false">
                                    Settings
                                </button>
                                <ul class="dropdown-menu rounded-0">
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

                            <button class='btn rounded-0 btn-sm tertiary-bg border border-secondary-subtle' @click='flashModal = true'>
                                Advanced
                            </button>

                        </div>
                    </div>
                    <div class="col-2 small pt-1 pe-3" v-if="device.active">
                        <div class="row">
                            <div class="col text-end">
                                Firmware version
                                <span class="text-body-secondary font-monospace me-2">{{ device.firmware_version_str }}</span>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <div class="container">
            <div class='row p-1' v-if="!collapsed">

                <!-- Rotation -->

                <div v-if='device.active' class='col-4 bg-body border border-secondary rounded-0'>
                    <div class="row pt-2">
                        <div class='col'>
                            <strong>Rotation</strong>
                        </div>
                        <div class='col'>
                            <div class="form-check form-check-reverse text-end">
                                <label class="form-check-label user-select-none" for="reverse_rotation">
                                    Reversed
                                </label>
                                <input class="form-check-input" type="checkbox" id="reverse_rotation" v-model="ui.cf_reverse_rotation">
                            </div>
                        </div>
                    </div>
                    <div class="row mb-3">
                        <div class='col'>
                            <div class="row">
                                <div class='col mx-3'>
                                    <HIDDropDown v-model:keycode="ui.key_clockwise" v-model:mod="ui.mod_clockwise" :show_mouse="supports_mouse">
                                        Clockwise
                                    </HIDDropDown>
                                </div>
                            </div>
                            <div class="row">
                                <div class='col mx-3'>
                                    <HIDDropDown v-model:keycode="ui.key_counterclockwise" v-model:mod="ui.mod_counterclockwise" :show_mouse="supports_mouse">
                                        Counter-clockwise
                                    </HIDDropDown>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Button -->

                <div v-if='device.active' class='col-4 bg-body border-top border-bottom border-secondary'>
                    <div class="row pt-2">
                        <div class="col mb-1">
                            <strong>Button</strong>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col mx-3">
                            <HIDDropDown v-model:keycode="ui.key_press" v-model:mod="ui.mod_press" :show_mouse="supports_mouse">Key</HIDDropDown>
                        </div>
                    </div>
                </div>

                <!-- LED -->

                <div v-if='device.active' class='col-4 bg-body border border-secondary rounded-0'>
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

    </div>

    <!-- Modals -->

    <Modal v-model="flashModal" maxwidth="550px" minwidth="550px" closeable header="Advanced Functions">
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
                    <li class="list-group-item font-monospace" :class='(idx !== error_messages.length - 1) ? "border-bottom" : ""'>
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
    padding-top: 0 !important;
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
